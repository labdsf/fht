#include <algorithm>
#include <math.h>
#include <unistd.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

/* Enables debug output. */
static bool dflag;

static void usage(void) {
	fputs("a.out [-d] input output\n", stderr);
	exit(1);
}

static unsigned bitrev_inc(unsigned i, unsigned N) {
	return (N & i) ? bitrev_inc(i ^ N, N >> 1) : i ^ N;
}

/*
 * Fast Hough transform for (x, shift) space
 * Height of img should be power of 2
 * Shift ranges from 0 to height of img
 */
static void FHT(const cv::Mat &img, cv::Mat &accum) {
	int width = img.cols;
	int height = img.rows;

	img.convertTo(accum, CV_32S);

	for(int h = 2; h <= height; h <<= 1)
	for(int y = 0; y < height; y += h) {
		int ss = 0;
		for(int s = 0; s < h / 2; s++) { /* Shift */
			int u = y + s;
			int d = u + h / 2;

			cv::Mat t = accum.row(d).clone();
			for(int i = 0; i < width; i++) {
				accum.at<int>(d, i) = (i + 2 * ss + 1 < width) ? (accum.at<int>(u, i) + t.at<int>(i + ss + 1)) : 0;
				accum.at<int>(u, i) = (i + 2 * ss     < width) ? (accum.at<int>(u, i) + t.at<int>(i + ss)    ) : 0;
			}
			ss = bitrev_inc(ss, h / 4);
		}
	}

	int k = 0;
	for(int i = 0; i < height; i++) {
		if(i < k) {
			/* swap */
			cv::Mat t = accum.row(i).clone();
			accum.row(k).copyTo(accum.row(i));
			t.copyTo(accum.row(k));
		}
		k = bitrev_inc(k, height / 2);
	}
}

/*
 * Run fast Hough transform to get results for both positive and negative shift.
 * Maps both results onto the (x, x + shift) space
 */
static void DoubleFHT(const cv::Mat &src, cv::Mat &dst) {
	cv::Mat acc;

	int w = src.cols;
	dst.create(w, w, CV_32S);
	FHT(src, acc);
	for(int x = 0; x < w; x++)  {
		int smax = w - x;
		for(int s = 0; s < smax; s++)
			dst.at<int>(x + s, x) = acc.at<int>(s, x);
	}

	cv::Mat f;
	cv::flip(src, f, 1);
	FHT(f, acc);

	for(int x = 0; x < w; x++)  {
		int smax = w - x;
		for(int s = 0; s < smax; s++)
			dst.at<int>(w - 1 - x - s, w - 1 - x) = acc.at<int>(s, x);
	}
}

int main(int argc, char *argv[]) {
	int ch;

	while ((ch = getopt(argc, argv, "d")) != -1) {
		if(ch == 'd')
			dflag = true;
		else
			usage();
	}
	
	argc -= optind;
	argv += optind;

	if(argc != 2)
		usage();

	/* Read image. */
	cv::Mat src = cv::imread(argv[0]);
	if(!src.data)
		return 1;

	cv::Mat edge;
	int d = 512;
{
	/* Resize. */
	cv::Mat resized;
	cv::resize(src, resized, cv::Size(d, d));

	/* Grayscale. */
	cv::Mat gray;
	cv::cvtColor(resized, gray, CV_BGR2GRAY);
	if(dflag)
		cv::imwrite("gray.jpg", gray);

	/* Blur. */
	cv::blur(gray, gray, cv::Size(3, 3));
	if(dflag)
		cv::imwrite("blur.jpg", gray);

	/* Threshold. */
	cv::Mat bw;
	cv::adaptiveThreshold(gray, bw, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 15, 2);
	if(dflag)
		cv::imwrite("bw.jpg", bw);

	/* Edge detection. */
	cv::Canny(bw, edge, 100, 300, 3);
	if(dflag)
		cv::imwrite("canny.jpg", edge);
}

	/* Hough */
	cv::Mat accum;
	DoubleFHT(edge, accum);
	if(dflag) {
		cv::Mat mnorm;
		cv::normalize(accum, mnorm, 0, 255, cv::NORM_MINMAX, CV_8U);
		cv::imwrite("hough1.jpg", mnorm);
	}

	cv::Mat image32f;
	accum.convertTo(image32f, CV_32F);

	for(int i = 0; i < image32f.rows; i++)
		cv::subtract(image32f.row(i), mean(image32f.row(i)), image32f.row(i));
	image32f = image32f.mul(image32f);

	cv::Mat sigma;
	cv::normalize(image32f, sigma, 0, 255, cv::NORM_MINMAX, CV_8U);
	if(dflag)
		cv::imwrite("res2.jpg", sigma);

	/* The vanishing point is above image, so calculate FHT only for positive shift. */
	FHT(sigma, accum);

	cv::Point maxloc;
	cv::minMaxLoc(accum, 0, 0, 0, &maxloc);

	cv::Scalar green(0, 255, 0);
	if(dflag) {
		cv::Mat mnorm;
		cv::normalize(accum, mnorm, 0, 255, cv::NORM_MINMAX, CV_8U);
		cv::cvtColor(mnorm, mnorm, CV_GRAY2BGR);
		cv::circle(mnorm, maxloc, 15, cv::Scalar(0, 255, 0), 3);
		cv::imwrite("hough2.jpg", mnorm);
	}

	cv::Point2f c(.5 * src.cols, .5 * src.rows);
	cv::Point2f v(maxloc.x * src.cols / (d - maxloc.y) - c.x,
	              .5 * src.rows - d * src.rows / (d - maxloc.y));

	if(dflag) {
		cv::line(src, cv::Point(0,        src.rows), c + v, green, 3);
		cv::line(src, cv::Point(src.cols, src.rows), c + v, green, 3);
		cv::Point p1(maxloc.x              * src.cols / d, 0);
		cv::Point p2((maxloc.x + maxloc.y) * src.cols / d, 0);
		cv::circle(src, p1, 15, green, 3);
		cv::circle(src, p2, 15, green, 3);
		cv::imwrite("debug.jpg", src);
	}

	/* Perspective transform. */
	double tiltangle = atan2(-v.x, -v.y); // - gamma

	double ta = sqrt(- c.y * cos(tiltangle) / v.y);
	double f = c.y / ta;
	double a = atan(ta); // - alpha

	cv::Mat camera = (cv::Mat_<double>(3, 3) <<
	f, 0, c.x,
	0, f, c.y,
	0, 0, 1);
	cv::Mat uncamera;
	cv::invert(camera, uncamera);

	cv::Mat R = (cv::Mat_<double>(3, 3) <<
	1, 0, 0,
	0, cos(a), -sin(a),
	0, sin(a), cos(a)
	) *
	(cv::Mat_<double>(3, 3) <<
	cos(tiltangle), -sin(tiltangle), 0,
	sin(tiltangle), cos(tiltangle), 0,
	0, 0, 1
	);

	cv::Mat A = camera * R * uncamera;

	/* Find bounding rect. */
	std::vector<cv::Point2f> corners(4);
	corners[0] = cv::Point2f(0,        0);
	corners[1] = cv::Point2f(src.cols, 0);
	corners[2] = cv::Point2f(0, src.rows);
	corners[3] = cv::Point2f(src.cols, src.rows);
	cv::perspectiveTransform(corners, corners, A);
	cv::Rect rect = cv::boundingRect(corners);

	cv::Mat shift = (cv::Mat_<double>(3, 3) <<
	1, 0, -rect.x,
	0, 1, -rect.y,
	0, 0, 1);

	cv::Mat result;
	cv::warpPerspective(src, result, shift * A, cv::Size(rect.width, rect.height), cv::INTER_LINEAR);
	cv::imwrite(argv[1], result);
}

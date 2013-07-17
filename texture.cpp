#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <fstream>
#include <iostream>
#include <ctime>
#include "debayer.h"
using namespace std;
using namespace cv;

Mat image, out;

int main ( int argc, char *argv[] )
{
	if (argc != 2)
	{
		cout << "Usage: ./debayer input.png" << endl;
		return -1;
	}

	image = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);

	out = Mat(image.size(), CV_8UC3);

	Debayer debayer(image.cols, image.rows);
	debayer.process(image.data, out.data);

	imwrite("output.png", out);

	return 0;
}

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/ml/ml.hpp>


// Train with LINEAR kernel Support Vector Classifier (C_SVC) with up to
// 1e7 iterations to achieve epsilon.
//
static cv::SVMParams makeSvmParams(void)
{
    static const int criteria = CV_TERMCRIT_ITER;
    static const int iterationCount = 10 * 1000 * 1000;
    static const double epsilon = std::numeric_limits<double>::epsilon();
    cv::SVMParams result;
    result.svm_type    = cv::SVM::C_SVC;
    result.kernel_type = cv::SVM::LINEAR;
    result.C           = 0.1;
    result.term_crit   = cv::TermCriteria(criteria, iterationCount, epsilon);
    return result;
}

// Train svm on data and labels.
//
static void trainSvm(cv::SVM &svm, const cv::Mat &data, const cv::Mat labels)
{
    static const cv::Mat zeroIdx;
    static const cv::Mat varIdx = zeroIdx;
    static const cv::Mat sampleIdx = zeroIdx;
    static const cv::SVMParams params = makeSvmParams();
    svm.train(data, labels, varIdx, sampleIdx, params);
}

// Return COUNT points of mostly SEPARABLE training data.
//
static cv::Mat_<float> makeData(int COUNT, const cv::Size &size)
{
    static cv::RNG rng(666);
    static const int SEPARABLE = 90;
    const int NONSEPARABLE = COUNT - SEPARABLE;
    const int cols = size.width;
    const int rows = size.height;
    cv::Mat_<float> result(COUNT, 2, CV_32FC1);
    const cv::Mat class1 = result.rowRange(0, SEPARABLE);
    rng.fill(class1.colRange(0, 1), cv::RNG::UNIFORM, 1.0, 0.4 * cols);
    rng.fill(class1.colRange(1, 2), cv::RNG::UNIFORM, 1.0, 1.0 * rows);
    const cv::Mat class2 = result.rowRange(NONSEPARABLE, COUNT);
    rng.fill(class2.colRange(0, 1), cv::RNG::UNIFORM, 0.6 * cols, 1.0 * cols);
    rng.fill(class2.colRange(1, 2), cv::RNG::UNIFORM, 1.0       , 1.0 * rows);
    const cv::Mat classX = result.rowRange(SEPARABLE, NONSEPARABLE);
    rng.fill(classX.colRange(0, 1), cv::RNG::UNIFORM, 0.4 * cols, 0.6 * cols);
    rng.fill(classX.colRange(1, 2), cv::RNG::UNIFORM, 1.0       , 1.0 * rows);
    return result;
}

// Return half the COUNT data labeled 1.0 and half labeled 2.0.
//
static cv::Mat_<float> labelData(int COUNT)
{
    cv::Mat_<float> result(COUNT, 1, CV_32FC1);
    result.rowRange(        0, COUNT / 2).setTo(1.0);
    result.rowRange(COUNT / 2, COUNT    ).setTo(2.0);
    return result;
}

// Draw on image the 2 classification regions predicted by svm.
// Draw class labeled 1.0 in green, and class 2.0 in blue.
//
static void drawRegions(cv::Mat &image, const cv::SVM &svm)
{
    static const cv::Vec3b green(  0, 100,  0);
    static const cv::Vec3b  blue(100,   0,  0);
    for (int i = 0; i < image.rows; ++i) {
        for (int j = 0; j < image.cols; ++j) {
            const cv::Mat sample = (cv::Mat_<float>(1,2) << i, j);
            const float response = svm.predict(sample);
            if (response == 1.0) {
                image.at<cv::Vec3b>(j, i) = green;
            } else if (response == 2.0) {
                image.at<cv::Vec3b>(j, i) = blue;
            } else {
                std::cerr << "Unexpected response from SVM::predict(): "
                          << response << std::endl;
                assert(!"expected response from SVM");
            }
        }
    }
}

// Draw training data as COUNT circles of radius 3 on image.
// Again, draw class 1.0 in green and class 2.0 in blue.
//
static void drawData(cv::Mat &image, int COUNT, const cv::Mat_<float> &data)
{
    static const cv::Scalar green(  0, 255,   0);
    static const cv::Scalar  blue(255,   0,   0);
    static const int radius = 3;
    static const int thickness = -1;
    static const int lineKind = 8;
    for (int i = 0; i < COUNT / 2; ++i) {
        const cv::Point center(data(i, 0), data(i, 1));
        cv::circle(image, center, radius, green, thickness, lineKind);
    }
    for (int i = COUNT / 2; i < COUNT; ++i) {
        const cv::Point center(data(i, 0), data(i, 1));
        cv::circle(image, center, radius, blue, thickness, lineKind);
    }

}

// Draw the support vectors in svm as circles of radius 6 in red.
//
static void drawSupportVectors(cv::Mat &image, const cv::SVM &svm)
{
    static const cv::Scalar red(0, 0, 255);
    static const int radius = 6;
    static const int thickness = 2;
    static const int lineKind = 8;
    const int count = svm.get_support_vector_count();
    std::cout << "support vector count == " << count << std::endl;
    for (int i = 0; i < count; ++i) {
        const float *const v = svm.get_support_vector(i);
        const cv::Point center(v[0], v[1]);
        std::cout << i << ": center == " << center << std::endl;
        cv::circle(image, center, radius, red, thickness, lineKind);
    }
}

int main(int, const char *[])
{
    static const int COUNT = 200;
    cv::Mat image = cv::Mat::zeros(512, 512, CV_8UC3);
    const cv::Mat_<float> data = makeData(COUNT, image.size());
    const cv::Mat_<float> labels = labelData(COUNT);
    std::cout << "Training SVM ... " << std::flush;
    cv::SVM svm;
    trainSvm(svm, data, labels);
    std::cout << "done." << std::endl;
    drawRegions(image, svm);
    drawData(image, COUNT, data);
    drawSupportVectors(image, svm);
    cv::imwrite("result.png", image);
    cv::imshow("SVM for Non-Linear Training Data", image);
    cv::waitKey(0);
}
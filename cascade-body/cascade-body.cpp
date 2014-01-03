#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>


static void showUsage(const char *av0)
{
    static const char bodies[]
        = "../resources/haarcascade_upperbody.xml";
    static const char faces[]
        = "../resources/haarcascade_frontalface_alt.xml";
    std::cerr << av0 << ": Use Haar cascade classifier to find people in video."
              << std::endl << std::endl
              << "Recoginize likely upper body "
              << "(head and shoulders) images in video."
              << std::endl
              << "Outline upper-body images in blue."
              << std::endl
              << "Within upper-body regions, recognize a face."
              << std::endl
              << "Outline faces in green."
              << std::endl
              << "Usage: " << av0 << " <camera> <faces>" << std::endl
              << std::endl
              << "Where: <camera> is an integer camera number." << std::endl
              << "       <bodies> is Haar training data (.xml) for bodies."
              << std::endl
              << "       <faces>  is Haar training data (.xml) for faces."
              << std::endl << std::endl
              << "Example: " << av0 << " 0 " << bodies << " \\ " << std::endl
              << "         " << faces << std::endl << std::endl;
}

// Return regions of interest detected by classifier in gray.
//
static void detectCascade(cv::CascadeClassifier &classifier,
                          const cv::Mat &gray,
                          std::vector<cv::Rect> &regions)
{
    static double scaleFactor = 1.1;
    static const int minNeighbors = 2;
    static const cv::Size minSize(30, 30);
    static const cv::Size maxSize;
    classifier.detectMultiScale(gray, regions, scaleFactor, minNeighbors,
                                cv::CASCADE_SCALE_IMAGE, minSize, maxSize);
}

// Draw rectangle r in color c on image i.
//
static void drawRectangle(cv::Mat &i, const cv::Scalar &c, const cv::Rect &r)
{
    static const int thickness = 4;
    static const int lineKind = 8;
    static const int shift = 0;
    cv::rectangle(i, r, c, thickness, lineKind, shift);
}

// Outline in blue any body in the frame.
// Outline in green any face in a body.
//
static void drawBody(cv::Mat &frame, const cv::Rect &body,
                     const std::vector<cv::Rect> &faces)
{
    static const cv::Scalar  blue(255,   0,   0);
    static const cv::Scalar green(  0, 255,   0);
    const cv::Point origin = body.tl();
    drawRectangle(frame, blue, body);
    const size_t faceCount = faces.size() > 1 ? 1 : faces.size();
    for (size_t i = 0; i < faceCount; ++i) {
        drawRectangle(frame, green, faces[i] + origin);
    }
}

// Detect any body in the frame.  Within the body's region of interest
// detect any face.
//
// Track regions of interest (ROI) here so boundaries can be properly
// outlined and offset in the frame.
//
static void displayBody(cv::Mat &frame,
                        cv::CascadeClassifier &bodyHaar,
                        cv::CascadeClassifier &faceHaar)
{
    static cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGB2GRAY);
    cv::equalizeHist(gray, gray);
    static std::vector<cv::Rect> bodies;
    detectCascade(bodyHaar, gray, bodies);
    for (size_t i = 0; i < bodies.size(); ++i) {
        const cv::Mat bodyROI = gray(bodies[i]);
        static std::vector<cv::Rect> faces;
        detectCascade(faceHaar, bodyROI, faces);
        drawBody(frame, bodies[i], faces);
    }
    cv::imshow("Viola-Jones-Lienhart Classifier", frame);
}


// Just cv::VideoCapture extended for convenience.  The const_cast<>()s
// work around the missing member const on cv::VideoCapture::get().
//
struct CvVideoCapture: cv::VideoCapture {

    double getFramesPerSecond() const {
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        const double fps = p->get(cv::CAP_PROP_FPS);
        return fps ? fps : 30.0;        // for MacBook iSight camera
    }

    int getFourCcCodec() const {
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        return p->get(cv::CAP_PROP_FOURCC);
    }

    std::string getFourCcCodecString() const {
        char result[] = "????";
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        const int code = p->getFourCcCodec();
        result[0] = ((code >>  0) & 0xff);
        result[1] = ((code >>  8) & 0xff);
        result[2] = ((code >> 16) & 0xff);
        result[3] = ((code >> 24) & 0xff);
        result[4] = ""[0];
        return std::string(result);
    }

    int getFrameCount() const {
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        return p->get(cv::CAP_PROP_FRAME_COUNT);
    }

    cv::Size getFrameSize() const {
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        const int w = p->get(cv::CAP_PROP_FRAME_WIDTH);
        const int h = p->get(cv::CAP_PROP_FRAME_HEIGHT);
        const cv::Size result(w, h);
        return result;
    }

    int getPosition(void) const {
        CvVideoCapture *const p = const_cast<CvVideoCapture *>(this);
        return p->get(cv::CAP_PROP_POS_FRAMES);
    }
    void setPosition(int p) { this->set(cv::CAP_PROP_POS_FRAMES, p); }

    CvVideoCapture(const std::string &fileName): VideoCapture(fileName) {}
    CvVideoCapture(int n): VideoCapture(n) {}
    CvVideoCapture(): VideoCapture() {}
};


int main(int ac, const char *av[])
{
    if (ac == 4) {
        int cameraId = 0;
        std::istringstream iss(av[1]); iss >> cameraId;
        cv::CascadeClassifier bodyHaar(av[2]);
        cv::CascadeClassifier faceHaar(av[3]);
        std::cout << av[0] << ": camera ID " << cameraId << std::endl
                  << av[0] << ": Body data from " << av[2] << std::endl
                  << av[0] << ": Face data from " << av[3] << std::endl;
        if (!bodyHaar.empty() && !faceHaar.empty()) {
            CvVideoCapture camera(cameraId);
            std::cout << std::endl << av[0] << ": Press any key to quit."
                      << std::endl << std::endl;
            const int msPerFrame = 1000.0 / camera.getFramesPerSecond();
            while (true) {
                static cv::Mat frame; camera >> frame;
                if (!frame.empty()) displayBody(frame, bodyHaar, faceHaar);
                const int c = cv::waitKey(msPerFrame);
                if (c != -1) break;
            }
            return 0;
        }
    }
    showUsage(av[0]);
    return 1;
}

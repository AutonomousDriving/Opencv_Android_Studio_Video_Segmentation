#include <jni.h>
#include "com_example_dedoo_segmen_MainActivity.h"
#include "com_example_dedoo_segmen_DetectionBasedTracker.h""
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include <string>
#include <vector>

#include <android/log.h>

#define LOG_TAG "FaceDetection/DetectionBasedTracker"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;
using namespace cv;

inline void vector_Rect_to_Mat(vector<Rect>& v_rect, Mat& mat)
{
    mat = Mat(v_rect, true);
}

JNIEXPORT void JNICALL
Java_com_example_dedoo_segmen_MainActivity_ConvertRGBtoGray(
        JNIEnv *env,
        jobject  instance,
        jlong matAddrInput,
        jlong matAddrResult){

    Mat &matInput = *(Mat *)matAddrInput;
    Mat &matResult = *(Mat *)matAddrResult;

    cvtColor(matInput, matResult, CV_RGBA2GRAY);

  }

class CascadeDetectorAdapter: public DetectionBasedTracker::IDetector
{
public:
    CascadeDetectorAdapter(cv::Ptr<cv::CascadeClassifier> detector):
            IDetector(),
            Detector(detector)
    {
        LOGD("CascadeDetectorAdapter::Detect::Detect");
        CV_Assert(detector);
    }

    void detect(const cv::Mat &Image, std::vector<cv::Rect> &objects)
    {
        LOGD("CascadeDetectorAdapter::Detect: begin");
        LOGD("CascadeDetectorAdapter::Detect: scaleFactor=%.2f, minNeighbours=%d, minObjSize=(%dx%d), maxObjSize=(%dx%d)", scaleFactor, minNeighbours, minObjSize.width, minObjSize.height, maxObjSize.width, maxObjSize.height);
        Detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize, maxObjSize);
        LOGD("CascadeDetectorAdapter::Detect: end");
    }

    virtual ~CascadeDetectorAdapter()
    {
        LOGD("CascadeDetectorAdapter::Detect::~Detect");
    }

private:
    CascadeDetectorAdapter();
    cv::Ptr<cv::CascadeClassifier> Detector;
};

struct DetectorAgregator
{
    cv::Ptr<CascadeDetectorAdapter> mainDetector;
    cv::Ptr<CascadeDetectorAdapter> trackingDetector;

    cv::Ptr<DetectionBasedTracker> tracker;
    DetectorAgregator(cv::Ptr<CascadeDetectorAdapter>& _mainDetector, cv::Ptr<CascadeDetectorAdapter>& _trackingDetector):
            mainDetector(_mainDetector),
            trackingDetector(_trackingDetector)
    {
        CV_Assert(_mainDetector);
        CV_Assert(_trackingDetector);

        DetectionBasedTracker::Parameters DetectorParams;
        tracker = makePtr<DetectionBasedTracker>(mainDetector, trackingDetector, DetectorParams);
    }
};

    JNIEXPORT jlong JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeCreateObject
            (JNIEnv *jenv, jclass, jstring jFileName, jint faceSize){
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeCreateObject enter");
        const char* jnamestr = jenv->GetStringUTFChars(jFileName, NULL);
        string stdFileName(jnamestr);
        jlong result = 0;

        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeCreateObject");

        try
        {
            cv::Ptr<CascadeDetectorAdapter> mainDetector = makePtr<CascadeDetectorAdapter>(
                    makePtr<CascadeClassifier>(stdFileName));
            cv::Ptr<CascadeDetectorAdapter> trackingDetector = makePtr<CascadeDetectorAdapter>(
                    makePtr<CascadeClassifier>(stdFileName));
            result = (jlong)new DetectorAgregator(mainDetector, trackingDetector);
            if (faceSize > 0)
            {
                mainDetector->setMinObjectSize(Size(faceSize, faceSize));
                //trackingDetector->setMinObjectSize(Size(faceSize, faceSize));
            }
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeCreateObject caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeCreateObject caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code of DetectionBasedTracker.nativeCreateObject()");
            return 0;
        }

        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeCreateObject exit");
        return result;
    }

    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeDestroyObject
            (JNIEnv *jenv, jclass, jlong thiz){

        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeDestroyObject");

        try
        {
            if(thiz != 0)
            {
                ((DetectorAgregator*)thiz)->tracker->stop();
                delete (DetectorAgregator*)thiz;
            }
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeestroyObject caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeDestroyObject caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code of DetectionBasedTracker.nativeDestroyObject()");
        }
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeDestroyObject exit");
    }


    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeStart
            (JNIEnv * jenv, jclass, jlong thiz)
    {
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeStart");

        try
        {
            ((DetectorAgregator*)thiz)->tracker->run();
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeStart caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeStart caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code of DetectionBasedTracker.nativeStart()");
        }
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeStart exit");
    }


    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeStop
            (JNIEnv * jenv, jclass, jlong thiz)
    {
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeStop");

        try
        {
            ((DetectorAgregator*)thiz)->tracker->stop();
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeStop caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeStop caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code of DetectionBasedTracker.nativeStop()");
        }
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeStop exit");
    }

    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeSetFaceSize
            (JNIEnv * jenv, jclass, jlong thiz, jint faceSize)
    {
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeSetFaceSize -- BEGIN");

        try
        {
            if (faceSize > 0)
            {
                ((DetectorAgregator*)thiz)->mainDetector->setMinObjectSize(Size(faceSize, faceSize));
                //((DetectorAgregator*)thiz)->trackingDetector->setMinObjectSize(Size(faceSize, faceSize));
            }
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeStop caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeSetFaceSize caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code of DetectionBasedTracker.nativeSetFaceSize()");
        }
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeSetFaceSize -- END");
    }


    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeDetect
            (JNIEnv * jenv, jclass, jlong thiz, jlong imageGray, jlong faces)
    {
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeDetect");

        try
        {
            vector<Rect> RectFaces;
            ((DetectorAgregator*)thiz)->tracker->process(*((Mat*)imageGray));
            ((DetectorAgregator*)thiz)->tracker->getObjects(RectFaces);
            *((Mat*)faces) = Mat(RectFaces, true);
        }
        catch(cv::Exception& e)
        {
            LOGD("nativeCreateObject caught cv::Exception: %s", e.what());
            jclass je = jenv->FindClass("org/opencv/core/CvException");
            if(!je)
                je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, e.what());
        }
        catch (...)
        {
            LOGD("nativeDetect caught unknown exception");
            jclass je = jenv->FindClass("java/lang/Exception");
            jenv->ThrowNew(je, "Unknown exception in JNI code DetectionBasedTracker.nativeDetect()");
        }
        LOGD("Java_org_opencv_samples_facedetect_DetectionBasedTracker_nativeDetect END");
    }
    JNIEXPORT void JNICALL Java_com_example_dedoo_segmen_DetectionBasedTracker_nativeWatershed
        (JNIEnv *, jclass, jint x1, jint y1, jint x2, jint y2, jlong inputImage, jlong outputImage )
    {
        Mat &matImageRGB = *(Mat *)inputImage;
        /*
        // markerMask is 1ch gray image -> set to 0/1 to show lines
        // imgGray is    3ch gray image
        Mat markerMask, imgGray;
        cvtColor(matImageRGB, markerMask, CV_RGB2GRAY);
        cvtColor(markerMask, imgGray, CV_GRAY2BGR);
        markerMask = Scalar::all(0);

        int i, j, compCount = 0;
        std::vector<std::vector<cv::Point> > contours; // vector of marker points
        std::vector<Vec4i> hierarchy;
        Scalar color(255);

        // line 1
        line(markerMask, Point((int)(x1 + (x2-x1)/4 ), (int)(y1 + (y2-y1)/4 )), Point((int)(x2- (x2-x1)/4 ), (int)(y1 + (y2-y1)/4 )), color, 3 );
        // line 2
        line(markerMask, Point((int)(x1 + (x2-x1)/4 ), (2*y1 - y2)),            Point((int)(x2- (x2-x1)/4 ), (2*y1 - y2)),            color, 3 );

        //markerMask 에 drawLIne한 뒤 찾으면 됨!
        findContours(markerMask, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

        Mat markers(markerMask.size(), CV_32S);
        markers = Scalar::all(0);
        int idx = 0;
        for (; idx >= 0; idx = hierarchy[idx][0], compCount++)
            drawContours(markers, contours, idx, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

        vector<Vec3b> colorTab;
        for (i = 0; i < compCount; i++)
        {
            int b = theRNG().uniform(0, 255);
            int g = theRNG().uniform(0, 255);
            int r = theRNG().uniform(0, 255);

            colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
        }

        Mat wshed(markers.size(), CV_8UC3);

        watershed(matImageRGB, markers);

        for (i = 0; i < markers.rows; i++)
            for (j = 0; j < markers.cols; j++)
            {
                int index = markers.at<int>(i, j);
                if (index == -1)
                    wshed.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
                else if (index <= 0 || index > compCount)
                    wshed.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
                else
                    wshed.at<Vec3b>(i, j) = colorTab[index - 1];
            }

        wshed = wshed*0.5 + imgGray*0.5;
        */
        Mat wshed(matImageRGB.size(), CV_8UC3);
        wshed = Scalar::all(128);
        *(Mat *)outputImage=wshed;
    }
}
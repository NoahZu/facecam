#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QCloseEvent>
#include <QMessageBox>
#include <QShortcut>

#include <SDL.h>

#include <algorithm>
#include <mutex>
#include <memory>
#include "opencv2/opencv.hpp"

#include "face_detector.h"
#include "face_detector_cascade.h"
#include "face_detector_ssd_resnet10.h"

#include "face_landmark_detector.h"
#include "face_landmark_detector_kazemi.h"
#include "face_landmark_detector_lbf.h"
#include "face_landmark_detector_syan_cnn.h"
#include "face_landmark_detector_syan_cnn_2.h"

#include "image_effect.h"
#include "effect_debug_info.h"
#include "effect_cloud.h"
#include "effect_rabbit_ears.h"
#include "effect_feather_hat.h"
#include "effect_tiger.h"
#include "effect_pink_glasses.h"

#include "file_storage.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void showCam();

protected:
    void closeEvent(QCloseEvent *event);
    void loadEffects();

private slots:
    void captureBtn_clicked();
    void openLibraryBtn_clicked();
    void cameraSelector_activated();
    void faceDetectorSelector_activated();
    void faceLandmarkDetectorSelector_activated();
    void effectList_onselectionchange();
    void showAboutBox();
    void refreshCams();
    
private:
    Ui::MainWindow *ui;

    // Current image
    // When user click "Capture", we take photo here then have it
    // to [Photos] folder
    cv::Mat current_img;
    std::mutex current_img_mutex;

    // File Storage
    ml_cam::FileStorage fs;


    QGraphicsPixmapItem pixmap;
    cv::VideoCapture video;

    // Face detectors
    std::vector<std::shared_ptr<FaceDetector>> face_detectors;
    int current_face_detector_index = -1; // Index of current face detector method in face_detectors

    // Face landmark detectors
    std::vector<std::shared_ptr<FaceLandmarkDetector>> face_landmark_detectors;
    int current_face_landmark_detector_index = -1; // Index of current face landmark detector method in face_detectors

    // Photo effects
    std::vector<std::shared_ptr<ImageEffect>> image_effects;
    std::vector<int> selected_effect_indices; // Indices of selected effect in image_effects


    // Camera to use
    int MAX_CAMS = 5; // Max number of camera supported. This number used to scan cameras
    int current_camera_index = 0;
    int selected_camera_index = 0;


public:
    void loadFaceDetectors();
    void loadFaceLandmarkDetectors();
    void setCurrentImage(const cv::Mat & img);
    cv::Mat getCurrentImage();
    void playShutter();

};

#endif // MAINWINDOW_H

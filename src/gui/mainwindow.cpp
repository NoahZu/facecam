#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    

    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(&pixmap);

    // Connect buttons
    connect(ui->captureBtn, SIGNAL(released()), this,
            SLOT(captureBtn_clicked()));
    connect(ui->infoBtn, SIGNAL(released()), this, SLOT(showAboutBox()));
    connect(ui->openLibraryBtn, SIGNAL(released()), this,
            SLOT(openLibraryBtn_clicked()));

    // Option selector events
    connect(ui->cameraSelector, SIGNAL(activated(int)), this,
            SLOT(cameraSelector_activated()));
    connect(ui->faceDetectorSelector, SIGNAL(activated(int)), this,
            SLOT(faceDetectorSelector_activated()));
    connect(ui->faceLandmarkDetectorSelector, SIGNAL(activated(int)), this,
            SLOT(faceLandmarkDetectorSelector_activated()));
    connect(ui->effectList, SIGNAL(itemSelectionChanged()), this,
            SLOT(effectList_onselectionchange()));

    // Use space to capture
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(shortcut, SIGNAL(activated()), this, SLOT(captureBtn_clicked()));

    // load effects to use in this project
    loadEffects();

    // Load Detectors
    loadFaceDetectors();
    loadFaceLandmarkDetectors();

    refreshCams();

    // Init Audio
    SDL_Init(SDL_INIT_AUDIO);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::playShutter() {
    SDL_AudioSpec wavSpec;
	Uint32 wavLength;
	Uint8 *wavBuffer;

	SDL_LoadWAV("sounds/shutter-fast.wav", &wavSpec, &wavBuffer, &wavLength);
	
	// open audio device
	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);

	// play audio
	int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
	SDL_PauseAudioDevice(deviceId, 0);

	// keep window open enough to hear the sound
	SDL_Delay(200);

	// clean up
	SDL_CloseAudioDevice(deviceId);
	SDL_FreeWAV(wavBuffer);
}

void MainWindow::captureBtn_clicked() {
    fs.saveImage(getCurrentImage());

    // *** Update icon of library to current image
    cv::Mat current_img = getCurrentImage();

    // Play sound file
    playShutter();

    // Create icon by cropping
    int width = current_img.cols;
    int height = current_img.rows;
    int padding_x = 0, padding_y = 0;
    int crop_size;
    if (width <= height) {
        crop_size = width;
        padding_y = (height - width) / 2;
    } else {
        crop_size = height;
        padding_x = (width - height) / 2;
    }

    cv::Mat crop =
        current_img(cv::Rect(padding_x, padding_y, crop_size, crop_size));
    cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);

    cv::Mat white_bg(50, 50, CV_8UC3, cv::Scalar(255, 255, 255));

    // Set icon for library
    QImage btn_icon = QImage((uchar *)crop.data, crop.cols, crop.rows, crop.step,
                            QImage::Format_RGB888);
    QImage btn_white_icon = QImage((uchar *)crop.data, crop.cols, crop.rows, crop.step,
                            QImage::Format_RGB888);
    ui->openLibraryBtn->setIcon(QIcon(QPixmap::fromImage(btn_white_icon)));
    ui->openLibraryBtn->setIcon(QIcon(QPixmap::fromImage(btn_icon)));
}

void MainWindow::openLibraryBtn_clicked() {
    std::string command;

#if defined(_WIN32)
    command = std::string("explorer ") + (fs.getPhotoPath() / fs.getLastSavedItem()).string();
#else
    command = std::string("./qimgv ") + (fs.getPhotoPath() / fs.getLastSavedItem()).string();
#endif

    std::system(command.c_str());
}

void MainWindow::faceDetectorSelector_activated() {
    current_face_detector_index =
        ui->faceDetectorSelector
            ->itemData(ui->faceDetectorSelector->currentIndex())
            .toInt();
}

void MainWindow::faceLandmarkDetectorSelector_activated() {
    current_face_landmark_detector_index =
        ui->faceLandmarkDetectorSelector
            ->itemData(ui->faceLandmarkDetectorSelector->currentIndex())
            .toInt();
}

void MainWindow::cameraSelector_activated() {
    selected_camera_index =
        ui->cameraSelector
            ->itemData(ui->cameraSelector->currentIndex())
            .toInt();
}

void MainWindow::effectList_onselectionchange() {
    QList<QListWidgetItem *> selected_effects = ui->effectList->selectedItems();

    // Save selected effects
    selected_effect_indices.clear();
    for (int i = 0; i < selected_effects.count(); ++i) {
        selected_effect_indices.push_back(
            selected_effects[i]->data(Qt::UserRole).toInt());
    }
}

void MainWindow::showAboutBox() {
    QMessageBox::about(this, "About Us",
                       "This camera app is built using C++, Qt, OpenCV "
                       "and Face detection - alignment algorithms to provide "
                       "funny decorations and filters.\n"
                       "Author:\n"
                       "\t- Viet Anh (vietanhdev.com)\n"
                       "Icons made by:\n"
                       "\t- https://www.flaticon.com/authors/smashicons\n"
                       "\t- https://www.flaticon.com/authors/roundicons\n"
                       "\t- https://www.freepik.com/free-vector/bunny-ears-nose-carnival-mask-photo_4015599.htm\n"
                       "\t- https://www.freepik.com/\n");
}

void MainWindow::showCam() {
    using namespace cv;

    if (!video.open(current_camera_index)) {
        QMessageBox::critical(
            this, "Camera Error",
            "Make sure you entered a correct camera index,"
            "<br>or that the camera is not being accessed by another program!");
        return;
    }

    Mat frame;
    while (true) {

        // User changed camera
        if (selected_camera_index != current_camera_index) {
            
            video.release();
            refreshCams();
            current_camera_index = selected_camera_index;
            video.open(current_camera_index);

        } else if (!video.isOpened()) {

            // Reset to default camera (0)
            refreshCams();
            current_camera_index = selected_camera_index =
            ui->cameraSelector
                ->itemData(ui->cameraSelector->currentIndex())
                .toInt();
            ui->cameraSelector->setCurrentIndex(0);
            video.open(current_camera_index);

        }

        // If we still cannot open camera, exit the program
        if (!video.isOpened()) {
            QMessageBox::critical(
            this, "Camera Error",
            "Make sure you entered a correct camera index,"
            "<br>or that the camera is not being accessed by another program!");
            exit(1);
        }

        video >> frame;
        if (!frame.empty()) {

            // Flip frame
            if (ui->flipCameraCheckBox->isChecked()) {
                flip(frame, frame, 1);
            }

            // Detect Faces
            Timer::time_duration_t face_detection_duration;
            Timer::time_duration_t face_alignment_duration;
            std::vector<LandMarkResult> faces;
            if (current_face_detector_index >= 0) {

                Timer::time_point_t start_time = Timer::getCurrentTime();
                faces = face_detectors[current_face_detector_index]->detect(frame);
                face_detection_duration = Timer::calcTimePassed(start_time);

                if (current_face_landmark_detector_index >= 0) {
                    start_time = Timer::getCurrentTime();
                    face_landmark_detectors[current_face_landmark_detector_index]->detect(frame, faces);
                    face_alignment_duration = Timer::calcTimePassed(start_time);
                }
            } else {  // Clear old results
                face_detection_duration = 0;
                face_alignment_duration = 0;
                faces.clear();
            }

            // Sort faces ascending by size  => Draw face filters for smaller faces behind those for bigger faces
            std::sort(std::begin(faces), std::end(faces),
                    [] (const auto& lhs, const auto& rhs) {
                return lhs.getFaceRect().area() < rhs.getFaceRect().area();
            });


            // Apply effects / filters
            for (size_t i = 0; i < selected_effect_indices.size(); ++i) {
                if (selected_effect_indices[i] >= 0) {

                    // Output FPS in debug
                    if (image_effects[selected_effect_indices[i]]->getName() == "Debug Info") {

                        float detection_fps = face_detection_duration == 0 ? 0 : 1000.0 / face_detection_duration;
                        float alignment_fps = face_alignment_duration == 0 ? 0 : 1000.0 / face_alignment_duration;

                        std::dynamic_pointer_cast<EffectDebugInfo>(image_effects[selected_effect_indices[i]])->outputFPS(detection_fps, alignment_fps);
                    }

                    image_effects[selected_effect_indices[i]]->apply(frame, faces);
                }
            }

            setCurrentImage(frame);

            // ### Show current image
            QImage qimg(frame.data, static_cast<int>(frame.cols),
                        static_cast<int>(frame.rows),
                        static_cast<int>(frame.step), QImage::Format_RGB888);
            pixmap.setPixmap(QPixmap::fromImage(qimg.rgbSwapped()));
            ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);
        }
        qApp->processEvents();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (video.isOpened()) {
        video.release();
    }
    QApplication::quit();
    exit(0);
}

// Load face detectors
void MainWindow::loadFaceDetectors() {
    
    // SSD - ResNet10 detector
    face_detectors.push_back(
        std::shared_ptr<FaceDetector>(new FaceDetectorSSDResNet10()));

    // Haar cascade detector
    face_detectors.push_back(
        std::shared_ptr<FaceDetector>(new FaceDetectorCascade("HaarCascade - OpenCV model", "models/detect_haarcascade/haarcascade_frontalface.xml")));

    // Haar cascade detector v1
    face_detectors.push_back(
        std::shared_ptr<FaceDetector>(new FaceDetectorCascade("HaarCascade - T02_27", "models/detect_haarcascade/T02_27.xml")));

    // LBF cascade detector
    // Pretrained model v6 - vietanhdev
    face_detectors.push_back(
        std::shared_ptr<FaceDetector>(new FaceDetectorCascade("LBFCascade - vietanhdev", "models/detect_lbfcascade/lbf_fact_detect_6.xml")));

    // Add detectors to selector box of GUI
    for (size_t i = 0; i < face_detectors.size(); ++i) {
        ui->faceDetectorSelector->addItem(
            QString::fromUtf8(face_detectors[i]->getDetectorName().c_str()),
            QVariant(static_cast<int>(i)));
    }
    // Add None option
    ui->faceDetectorSelector->addItem("None", -1);

    current_face_detector_index = 0;  // set default face detector method
}


// Load face landmark detectors
void MainWindow::loadFaceLandmarkDetectors() {

    // Landmark LBF
    face_landmark_detectors.push_back(
        std::shared_ptr<FaceLandmarkDetector>(new FaceLandmarkDetectorLBF()));

    // Landmark Kazemi
    face_landmark_detectors.push_back(
        std::shared_ptr<FaceLandmarkDetector>(new FaceLandmarkDetectorKazemi()));

    
    // Landmark Sy An CNN
    face_landmark_detectors.push_back(
        std::shared_ptr<FaceLandmarkDetector>(new FaceLandmarkDetectorSyanCNN()));

    
    // Landmark Sy An CNN 2
    face_landmark_detectors.push_back(
        std::shared_ptr<FaceLandmarkDetector>(new FaceLandmarkDetectorSyanCNN2()));
        

    // Add detectors to selector box of GUI
    for (size_t i = 0; i < face_landmark_detectors.size(); ++i) {
        ui->faceLandmarkDetectorSelector->addItem(
            QString::fromUtf8(face_landmark_detectors[i]->getDetectorName().c_str()),
            QVariant(static_cast<int>(i)));
    }
    // Add None option
    ui->faceLandmarkDetectorSelector->addItem("None", -1);

    current_face_landmark_detector_index = 0;  // set default face detector method
}

void MainWindow::loadEffects() {
    // Effect: Debug
    image_effects.push_back(
        std::shared_ptr<ImageEffect>(new EffectDebugInfo()));

    // Effect: Raining Cloud
    image_effects.push_back(std::shared_ptr<ImageEffect>(new EffectCloud()));

    // Effect: Pink Glasses
    image_effects.push_back(std::shared_ptr<ImageEffect>(new EffectPinkGlasses()));

    // Effect: Rabbit Ears
    image_effects.push_back(std::shared_ptr<ImageEffect>(new EffectRabbitEars()));

    // Effect: Tiger
    image_effects.push_back(std::shared_ptr<ImageEffect>(new EffectTiger()));

    // Effect: Feather Hat
    image_effects.push_back(std::shared_ptr<ImageEffect>(new EffectFeatherHat()));

    // Add "No Effect"
    QListWidgetItem *new_effect = new QListWidgetItem(
        QIcon(":/resources/images/no-effect.png"), "No Effect");
    new_effect->setData(Qt::UserRole, QVariant(static_cast<int>(-1)));
    ui->effectList->addItem(new_effect);

    for (size_t i = 0; i < image_effects.size(); ++i) {
        std::string effect_name = image_effects[i]->getName();
        QString effect_name_qs = QString::fromUtf8(effect_name.c_str());
        QListWidgetItem *new_effect = new QListWidgetItem(
            QIcon(QPixmap::fromImage(
                ml_cam::Mat2QImage(image_effects[i]->getIcon()))),
            effect_name_qs);
        new_effect->setData(Qt::UserRole, QVariant(static_cast<int>(i)));
        ui->effectList->addItem(new_effect);
    }
}


void MainWindow::refreshCams() {

    // Get the number of camera available
    cv::VideoCapture temp_camera;
    ui->cameraSelector->clear();
    for (int i = 0; i < MAX_CAMS; ++i) {
        cv::VideoCapture temp_camera(i);
        bool fail = (!temp_camera.isOpened());
        temp_camera.release();

        // If we can open camera, add new camera to list
        if (!fail) {
            ui->cameraSelector->addItem(
            QString::fromUtf8((std::string("CAM") + std::to_string(i)).c_str()),
            QVariant(static_cast<int>(i)));
        }
    }

    // Select current camera
    int index = ui->cameraSelector->findData(selected_camera_index);
    if ( index != -1 ) { // -1 for not found
        ui->cameraSelector->setCurrentIndex(index);
    }
}

void MainWindow::setCurrentImage(const cv::Mat &img) {
    std::lock_guard<std::mutex> guard(current_img_mutex);
    current_img = img.clone();
}

cv::Mat MainWindow::getCurrentImage() {
    std::lock_guard<std::mutex> guard(current_img_mutex);
    return current_img.clone();
}
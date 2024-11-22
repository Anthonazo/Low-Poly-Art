    #include <opencv2/opencv.hpp>
    #include <iostream>
    #include <vector>

    using namespace cv;
    using namespace std;
    
    // bool isPointInCircumcircle(Point p, const Triangle& t) {
    //     Mat_<double> mat = (Mat_<double>(4, 4) << 
    //         t.p1.x, t.p1.y, t.p1.x * t.p1.x + t.p1.y * t.p1.y, 1,
    //         t.p2.x, t.p2.y, t.p2.x * t.p2.x + t.p2.y * t.p2.y, 1,
    //         t.p3.x, t.p3.y, t.p3.x * t.p3.x + t.p3.y * t.p3.y, 1,
    //         p.x, p.y, p.x * p.x + p.y * p.y, 1);
    //     return determinant(mat) > 0;
    // }

    int cannyThreshold1 = 100;
    int cannyThreshold2 = 300;
    int skip = 5;  // Controla la densidad de puntos
    double scaleFactor = 0.5;  // Escala para redimensionar la imagen
    cv::Mat mask;


    // Estructura de Triángulo
    struct Triangle {
        cv::Point2f p1, p2, p3;
        Triangle(cv::Point2f a, cv::Point2f b, cv::Point2f c) : p1(a), p2(b), p3(c) {}
    };

    bool isPointInCircumcircle(cv::Point2f p, const Triangle& t) {
        double ax = t.p1.x - p.x;
        double ay = t.p1.y - p.y;
        double bx = t.p2.x - p.x;
        double by = t.p2.y - p.y;
        double cx = t.p3.x - p.x;
        double cy = t.p3.y - p.y;

        double det = (ax * ax + ay * ay) * (bx * cy - by * cx) -
                    (bx * bx + by * by) * (ax * cy - ay * cx) +
                    (cx * cx + cy * cy) * (ax * by - ay * bx);

        return det > 0;
    }

    // Inserta un nuevo punto en la triangulación y actualiza los triángulos
    vector<Triangle> insertPoint(vector<Triangle>& triangles, cv::Point2f newPoint) {
        vector<Triangle> newTriangles;
        vector<pair<cv::Point2f, cv::Point2f>> edges;

        for (const auto& t : triangles) {
            if (isPointInCircumcircle(newPoint, t)) {
                edges.push_back({t.p1, t.p2});
                edges.push_back({t.p2, t.p3});
                edges.push_back({t.p3, t.p1});
            } else {
                newTriangles.push_back(t);
            }
        }

        // Eliminar aristas duplicadas de los triángulos
        for (size_t i = 0; i < edges.size(); i++) {
            for (size_t j = i + 1; j < edges.size(); j++) {
                if ((edges[i].first == edges[j].second && edges[i].second == edges[j].first) ||
                    (edges[i].first == edges[j].first && edges[i].second == edges[j].second)) {
                    edges[i] = edges[j] = {cv::Point2f(-1, -1), cv::Point2f(-1, -1)}; // Marcar como duplicados
                }
            }
        }

        // Crear nuevos triángulos con los bordes restantes
        for (const auto& e : edges) {
            if (e.first != cv::Point2f(-1, -1) && e.second != cv::Point2f(-1, -1)) {
                newTriangles.push_back(Triangle(e.first, e.second, newPoint));
            }
        }

        return newTriangles;
    }

    // Aplica el efecto Low Poly a una imagen
    void applyLowPolyEffect(const cv::Mat& img, cv::Mat& output, cv::Mat& edges) {
        cv::Mat resizedImg;
        resize(img, resizedImg, cv::Size(), scaleFactor, scaleFactor);

        Canny(resizedImg, edges, cannyThreshold1, cannyThreshold2);

        vector<cv::Point2f> points;
        for (int y = 0; y < edges.rows; y += skip) {
            for (int x = 0; x < edges.cols; x += skip) {
                if (edges.at<uchar>(y, x) > 0) points.push_back(cv::Point2f(x, y));
            }
        }

        cv::Mat localPoints = cv::Mat::zeros(resizedImg.size(), CV_8UC3); // Cambiar a CV_8UC3 para colores


        vector<Triangle> triangles = {
                Triangle(cv::Point2f(0, 0), cv::Point2f(resizedImg.cols - 1, 0), cv::Point2f(0, resizedImg.rows - 1)),
                Triangle(cv::Point2f(resizedImg.cols - 1, 0), cv::Point2f(resizedImg.cols - 1, resizedImg.rows - 1), cv::Point2f(0, resizedImg.rows - 1))
        };

        // Insertar puntos
        for (const cv::Point2f& p : points) {
            triangles = insertPoint(triangles, p);
        }

        for (const auto& t : triangles) {
            std::array<cv::Point, 3> triangleInt = {
                cv::Point(cvRound(t.p1.x), cvRound(t.p1.y)),
                cv::Point(cvRound(t.p2.x), cvRound(t.p2.y)),
                cv::Point(cvRound(t.p3.x), cvRound(t.p3.y))
            };

            cv::Rect boundingBox = boundingRect(triangleInt);
            mask = cv::Mat::zeros(boundingBox.size(), CV_8UC1);

            std::array<cv::Point, 3> roiTriangle = {
                triangleInt[0] - boundingBox.tl(),
                triangleInt[1] - boundingBox.tl(),
                triangleInt[2] - boundingBox.tl()
            };

            fillConvexPoly(mask, roiTriangle, 255);

            cv::Mat roiImg = resizedImg(boundingBox);
            cv::Scalar meanColor = cv::mean(roiImg, mask);

            fillConvexPoly(localPoints, triangleInt, 
                cv::Scalar(meanColor[0], meanColor[1], meanColor[2]), cv::LINE_AA);
        }

        resize(localPoints, output, img.size(), 0, 0, cv::INTER_LINEAR);
    }


int main() {

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "No se pudo abrir la cámara" << endl;
        return -1;
    }

    Mat frame;
    namedWindow("Low Poly Effect", WINDOW_AUTOSIZE);
    createTrackbar("Canny Threshold 1", "Low Poly Effect", &cannyThreshold1, 1500);
    createTrackbar("Canny Threshold 2", "Low Poly Effect", &cannyThreshold2, 1500);
    createTrackbar("Skip", "Low Poly Effect", &skip, 100);

    int64 startTime = cv::getTickCount(); 
    int frameCount = 0;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cout << "No se pudo capturar el frame" << endl;
            break;
        }

        frameCount++; 

        Mat output, edges;
        applyLowPolyEffect(frame, output, edges);

        int64 currentTime = cv::getTickCount();
        double fps = frameCount / ((currentTime - startTime) / cv::getTickFrequency());
        frameCount = 0;
        startTime = currentTime;
        putText(output, "FPS: " + to_string(fps), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
        
        imshow("Low Poly Effect", output);
        imshow("Original", frame);

        char key = (char)waitKey(1);
        if (key == 27) break; 
    }

    //   Mat img = imread("San-Basilio.webp");
    //     if (img.empty()) {
    //         cout << "No se pudo cargar la imagen" << endl;
    //         return -1;
    //     }
    //     namedWindow("Low Poly Effect", WINDOW_AUTOSIZE);
    //     createTrackbar("Canny Threshold 1", "Low Poly Effect", &cannyThreshold1, 1500);
    //     createTrackbar("Canny Threshold 2", "Low Poly Effect", &cannyThreshold2, 1500);
    //     createTrackbar("Skip", "Low Poly Effect", &skip, 100);
    //     Mat output, edges;
    //     applyLowPolyEffect(img, output, edges);
    //     imshow("Low Poly Effect", output);
    //     waitKey(0);

    return 0;
}
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Parámetros de configuración
int cannyThreshold1 = 100;
int cannyThreshold2 = 300;
int skip = 5;  // Controla la densidad de puntos
double scaleFactor = 0.5;  // Escala para redimensionar la imagen

// Estructura de Triángulo
struct Triangle {
    Point2f p1, p2, p3;
    Triangle(Point2f a, Point2f b, Point2f c) : p1(a), p2(b), p3(c) {}
};

// Verifica si un punto está dentro del circuncírculo de un triángulo
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
vector<Triangle> insertPoint(vector<Triangle>& triangles, Point2f newPoint) {
    vector<Triangle> newTriangles;
    vector<pair<Point2f, Point2f>> edges;

    for (const auto& t : triangles) {
        if (isPointInCircumcircle(newPoint, t)) {
            edges.push_back({t.p1, t.p2});
            edges.push_back({t.p2, t.p3});
            edges.push_back({t.p3, t.p1});
        } else {
            newTriangles.push_back(t);
        }
    }

    // Eliminar aristas duplicadas
    for (size_t i = 0; i < edges.size(); i++) {
        for (size_t j = i + 1; j < edges.size(); j++) {
            if ((edges[i].first == edges[j].second && edges[i].second == edges[j].first) ||
                (edges[i].first == edges[j].first && edges[i].second == edges[j].second)) {
                edges[i] = edges[j] = {Point2f(-1, -1), Point2f(-1, -1)};
            }
        }
    }

    // Crear nuevos triángulos con bordes válidos
    for (const auto& e : edges) {
        if (e.first != Point2f(-1, -1) && e.second != Point2f(-1, -1)) {
            newTriangles.emplace_back(e.first, e.second, newPoint);
        }
    }

    return newTriangles;
}

// Aplica el efecto Low Poly a una imagen
void applyLowPolyEffect(const Mat& resizedImg, Mat& output) {
    Mat edges, mask, localPoints = Mat::zeros(resizedImg.size(), resizedImg.type());
    Canny(resizedImg, edges, cannyThreshold1, cannyThreshold2);

    // Detectar puntos de borde
    vector<Point2f> points;
    for (int y = 0; y < edges.rows; y += skip) {
        for (int x = 0; x < edges.cols; x += skip) {
            if (edges.at<uchar>(y, x) > 0) points.push_back(Point2f(x, y));
        }
    }

    // Crear triangulación inicial
    vector<Triangle> triangles = {
        Triangle(Point2f(0, 0), Point2f(resizedImg.cols - 1, 0), Point2f(0, resizedImg.rows - 1)),
        Triangle(Point2f(resizedImg.cols - 1, 0), Point2f(resizedImg.cols - 1, resizedImg.rows - 1), Point2f(0, resizedImg.rows - 1))
    };

    for (const Point2f& p : points) {
        triangles = insertPoint(triangles, p);
    }

    // Renderizar triangulación
    for (const auto& t : triangles) {
        array<Point, 3> triangleInt = {
            Point(cvRound(t.p1.x), cvRound(t.p1.y)),
            Point(cvRound(t.p2.x), cvRound(t.p2.y)),
            Point(cvRound(t.p3.x), cvRound(t.p3.y))
        };

        Rect boundingBox = boundingRect(triangleInt);
        mask = Mat::zeros(boundingBox.size(), CV_8UC1);

        array<Point, 3> roiTriangle = {
            triangleInt[0] - boundingBox.tl(),
            triangleInt[1] - boundingBox.tl(),
            triangleInt[2] - boundingBox.tl()
        };

        fillConvexPoly(mask, roiTriangle, 255);
        Mat roiImg = resizedImg(boundingBox);
        Scalar meanColor = mean(roiImg, mask);

        fillConvexPoly(localPoints, triangleInt, Scalar(meanColor[0], meanColor[1], meanColor[2]), LINE_AA);
    }

    localPoints.copyTo(output);
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

        Mat resizedImg, output;
        resize(frame, resizedImg, Size(), scaleFactor, scaleFactor);

        applyLowPolyEffect(resizedImg, output);

        resize(output, output, frame.size(), 0, 0, INTER_LINEAR);

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

    return 0;
}
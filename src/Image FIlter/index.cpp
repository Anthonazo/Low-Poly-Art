#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

int cannyThreshold1 = 100;
int cannyThreshold2 = 200;
int skip = 15; 

struct Triangle {
    Point2f p1, p2, p3;
    Triangle(Point2f a, Point2f b, Point2f c) : p1(a), p2(b), p3(c) {}
};

bool isPointInCircumcircle(Point2f p, const Triangle& t) {
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

    for (size_t i = 0; i < edges.size(); i++) {
        for (size_t j = i + 1; j < edges.size(); j++) {
            if ((edges[i].first == edges[j].second && edges[i].second == edges[j].first) ||
                (edges[i].first == edges[j].first && edges[i].second == edges[j].second)) {
                edges[i] = edges[j] = {Point2f(-1, -1), Point2f(-1, -1)};
            }
        }
    }

    for (const auto& e : edges) {
        if (e.first != Point2f(-1, -1) && e.second != Point2f(-1, -1)) {
            newTriangles.push_back(Triangle(e.first, e.second, newPoint));
        }
    }
    return newTriangles;
}

void applyLowPolyEffect(const Mat& img, Mat& output, Mat& edges, Mat& drawPoints, Mat& imageTriangles) {
    Canny(img, edges, cannyThreshold1, cannyThreshold2);

    vector<Point2f> points;
    for (int y = 0; y < edges.rows; y += skip) {
        for (int x = 0; x < edges.cols; x += skip) {
            if (edges.at<uchar>(y, x) > 0) points.push_back(Point2f(x, y));
        }
    }

    vector<Triangle> triangles = {
        Triangle(Point2f(0, 0), Point2f(img.cols - 1, 0), Point2f(0, img.rows - 1)),
        Triangle(Point2f(img.cols - 1, 0), Point2f(img.cols - 1, img.rows - 1), Point2f(0, img.rows - 1))
    };

    output = Mat::zeros(img.size(), CV_8UC3);
    drawPoints = Mat::zeros(img.size(), CV_8UC3);

    for (const Point2f& p : points) {
        imageTriangles = Mat::zeros(img.size(), CV_8UC3);  // Limpia la imagen antes de cada iteración
        for (const auto& t : triangles) {
            vector<Point> pts = {t.p1, t.p2, t.p3};
            polylines(imageTriangles, pts, true, Scalar(0, 255, 0), 2);
        }

        imshow("Triángulos Progreso", imageTriangles);
        if (waitKey(50) == 27) break;

        triangles = insertPoint(triangles, p);
    }

    for (const auto& t : triangles) {
        std::array<Point, 3> triangleInt = {
            Point(cvRound(t.p1.x), cvRound(t.p1.y)),
            Point(cvRound(t.p2.x), cvRound(t.p2.y)),
            Point(cvRound(t.p3.x), cvRound(t.p3.y))
        };

        Rect boundingBox = boundingRect(triangleInt);
        Mat mask = Mat::zeros(boundingBox.size(), CV_8UC1);

        std::array<Point, 3> roiTriangle = {
            triangleInt[0] - boundingBox.tl(),
            triangleInt[1] - boundingBox.tl(),
            triangleInt[2] - boundingBox.tl()
        };

        fillConvexPoly(mask, roiTriangle, 255);
        Mat roiImg = img(boundingBox);
        Scalar meanColor = mean(roiImg, mask);

        fillConvexPoly(output, triangleInt, Scalar(meanColor[0], meanColor[1], meanColor[2]), LINE_AA);
    }

    for (const auto& p : points) {
        circle(drawPoints, p, 3, Scalar(0, 0, 255), FILLED, LINE_AA);
    }
}

int main() {
    Mat img = imread("San-Basilio.webp");
    if (img.empty()) {
        cout << "No se pudo cargar la imagen" << endl;
        return -1;
    }

    Mat output, edges, drawPoints, imageTriangles;
    applyLowPolyEffect(img, output, edges, drawPoints, imageTriangles);

    imshow("Original", img);
    imshow("Low Poly Effect", output);
    imshow("Points", drawPoints);
    imshow("Edges", edges);
    waitKey(0);

    return 0;
}
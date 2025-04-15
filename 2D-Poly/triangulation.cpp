#include "triangulation.h"

using namespace std;

bool operator==(const Edge &e1, const Edge &e2)
{
    return (e1.a == e2.a && e1.b == e2.b) || (e1.a == e2.b && e1.b == e2.a);
}

// 計算三個點的有向面積（正值表示逆時針排列）
double orientation(const cv::Point &a, const cv::Point &b, const cv::Point &c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// 判斷點 p 是否在由 (a, b, c) 構成三角形的外接圓內
// 此函式要求 a, b, c 為逆時針排列，則 determinant > 0 表示 p 落在外接圓內
bool inCircumcircle(const cv::Point &p, const cv::Point &a, const cv::Point &b, const cv::Point &c)
{
    double ax = a.x - p.x;
    double ay = a.y - p.y;
    double bx = b.x - p.x;
    double by = b.y - p.y;
    double cx = c.x - p.x;
    double cy = c.y - p.y;

    double det = (ax * ax + ay * ay) * (bx * cy - cx * by) -
                 (bx * bx + by * by) * (ax * cy - cx * ay) +
                 (cx * cx + cy * cy) * (ax * by - bx * ay);
    return det > 0;
}



DTImage::DTImage(int w, int h) : width(w), height(h) {};
DTImage::DTImage(int w, int h, const std::vector<cv::Point>& ps) : width(w), height(h), points(ps)
{
    delaunayTriangulation(points);
};

// 使用 Bowyer–Watson 演算法構造 Delaunay 三角剖分
// 傳入的 points 陣列會暫時加入超大三角形的頂點，供演算法使用。
// 為避免在最後輸出結果時混淆，我們約定原始點數為 original_n，超大三角形的頂點索引皆大於等於 original_n。
void DTImage::delaunayTriangulation(const std::vector<cv::Point>& ps)
{
    this->points = ps;
    int original_n = points.size();

    // 計算點集合的邊界
    double minX = points[0].x, minY = points[0].y;
    double maxX = points[0].x, maxY = points[0].y;
    for (const auto& p : points)
    {
        if (p.x < minX)
            minX = p.x;
        if (p.x > maxX)
            maxX = p.x;
        if (p.y < minY)
            minY = p.y;
        if (p.y > maxY)
            maxY = p.y;
    }
    double dx = maxX - minX, dy = maxY - minY;
    double deltaMax = max(dx, dy);
    double midx = (minX + maxX) / 2.0;
    double midy = (minY + maxY) / 2.0;

    // 建立一個足夠大的超大三角形，保證所有輸入點都包含在內
    cv::Point p1 = { int(midx - 2 * deltaMax), int(midy - deltaMax) };
    cv::Point p2 = { int(midx), int(midy + 2 * deltaMax) };
    cv::Point p3 = { int(midx + 2 * deltaMax), int(midy - deltaMax) };

    // 將超大三角形的頂點加入點集合中
    points.push_back(p1);
    points.push_back(p2);
    points.push_back(p3);
    int idx_p1 = points.size() - 3;
    int idx_p2 = points.size() - 2;
    int idx_p3 = points.size() - 1;

    // 初始三角形即為超大三角形
    triangles.push_back({ idx_p1, idx_p2, idx_p3 });

    // 將每個原始點依序插入
    // 注意：這邊只迭代原始點（索引 0 ~ original_n-1）
    for (int i = 0; i < original_n; i++)
    {
        cv::Point p = points[i];
        vector<Triangle> badTriangles; // 儲存外接圓包含 p 的三角形
        vector<Edge> polygon;          // 儲存多邊形邊界

        // 找出所有外接圓包含 p 的三角形
        for (int j = 0; j < triangles.size(); j++)
        {
            Triangle tri = triangles[j];
            cv::Point a = points[tri.a];
            cv::Point b = points[tri.b];
            cv::Point c = points[tri.c];
            // 若三角形不是逆時針排列則調整之
            if (orientation(a, b, c) < 0)
                swap(b, c);
            if (inCircumcircle(p, a, b, c))
                badTriangles.push_back(tri);
        }

        // 從所有不適合的三角形中找到「多邊形孔」的邊界
        // 方法：對每個不適合三角形的每一邊，如果該邊在其他不適合三角形中沒有出現過，則此邊屬於邊界
        for (size_t i_tri = 0; i_tri < badTriangles.size(); i_tri++)
        {
            // 三角形有三個邊
            Edge edges[3] = {
                {badTriangles[i_tri].a, badTriangles[i_tri].b},
                {badTriangles[i_tri].b, badTriangles[i_tri].c},
                {badTriangles[i_tri].c, badTriangles[i_tri].a} };
            for (int e = 0; e < 3; e++)
            {
                bool shared = false;
                // 與其他不適合三角形的邊做比對
                for (size_t j_tri = 0; j_tri < badTriangles.size(); j_tri++)
                {
                    if (i_tri == j_tri)
                        continue;
                    Edge otherEdges[3] = {
                        {badTriangles[j_tri].a, badTriangles[j_tri].b},
                        {badTriangles[j_tri].b, badTriangles[j_tri].c},
                        {badTriangles[j_tri].c, badTriangles[j_tri].a} };
                    for (int k = 0; k < 3; k++)
                    {
                        if (edges[e] == otherEdges[k])
                        {
                            shared = true;
                            break;
                        }
                    }
                    if (shared)
                        break;
                }
                if (!shared)
                    polygon.push_back(edges[e]);
            }
        }

        // 刪除所有外接圓包含 p 的三角形
        triangles.erase(remove_if(triangles.begin(), triangles.end(),
            [p, this](const Triangle& tri)
            {
                cv::Point a = this->points[tri.a];
                cv::Point b = this->points[tri.b];
                cv::Point c = this->points[tri.c];
                if (orientation(a, b, c) < 0)
                    swap(b, c);
                return inCircumcircle(p, a, b, c);
            }),
            triangles.end());

        // 將新點 p 與多邊形邊界各邊連結形成新的三角形
        for (auto& edge : polygon)
        {
            triangles.push_back({ edge.a, edge.b, i });
        }
    }

    // 刪除所有三角形中出現超大三角形頂點的那些（這些三角形是虛構出來的輔助部份）
    triangles.erase(remove_if(triangles.begin(), triangles.end(),
        [original_n](const Triangle& tri)
        {
            return (tri.a >= original_n || tri.b >= original_n || tri.c >= original_n);
        }),
        triangles.end());
}

void DTImage::drawLine()
{
    lineImg = cv::Mat(this->height, this->width, CV_8UC3, cv::Scalar(255, 255, 255));

    for (auto& tri : triangles)
    {
        cv::line(lineImg, points[tri.a], points[tri.b], cv::Scalar(0, 0, 0), 1);
        cv::line(lineImg, points[tri.a], points[tri.c], cv::Scalar(0, 0, 0), 1);
        cv::line(lineImg, points[tri.b], points[tri.c], cv::Scalar(0, 0, 0), 1);
    }
}

void DTImage::drawColor(const cv::Mat& origImg)
{
    // 施工中 ...
};

int DTImage::getNumTriangles() const { return triangles.size(); }

void DTImage::printTriangles() const
{
    for (auto& tri : triangles)
        cout << "Triangle: ("
            << points[tri.a].x << ", " << points[tri.a].y << ") - ("
            << points[tri.b].x << ", " << points[tri.b].y << ") - ("
            << points[tri.c].x << ", " << points[tri.c].y << ")\n";
};

void DTImage::writeLineImg(const std::string filename) const
{ 
    cv::imwrite(filename, this->lineImg);
};

void DTImage::writeColoredImg(const std::string filename) const
{
    cv::imwrite(filename, this->coloredImg);
};

int main()
{
    // 前三個數字是 點的數量、影像寬度、影像高度
    ifstream fin("../../testData/01.txt");
    int n, w, h;
    fin >> n >> w >> h;

    // 後面 2n 個數字是 n 個座標點
    vector<cv::Point> points(n);
    for (int i = 0; i < n; i++)
        fin >> points[i].x >> points[i].y;
    fin.close();

    // 執行 Delaunay 三角剖分
    DTImage dtImg(w, h, points);

    dtImg.getNumTriangles();
    dtImg.printTriangles();
    dtImg.drawLine();
    dtImg.writeLineImg("LineImg01.jpg");
    
    return 0;
}

#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <vector>

struct Ray
{
    sf::Vector2f src, dir;
    sf::Color col;
    float len = std::numeric_limits<float>::max();
    int refl;

    Ray(sf::Vector2f src, sf::Vector2f dir, sf::Color col = sf::Color::White, int refl = -1) : src(src), dir(dir), col(col), refl(refl) {}
};
struct Reflector
{
    sf::Vector2f a, b;
    float rIdx;
    int reflCount = 0;

    Reflector(sf::Vector2f a, sf::Vector2f b, float rIdx) : a(a), b(b), rIdx(rIdx) {}
};

void setVertices(sf::VertexArray &va, std::vector<Ray> rays, std::vector<Reflector> refls)
{
    int i = 0;
    while (i < rays.size())
    {
        va[2 * i] = sf::Vertex(rays[i].src, rays[i].col);
        va[2 * i + 1] = sf::Vertex(rays[i].src + rays[i].dir * rays[i].len, rays[i].col);
        i++;
    }
    while (i < rays.size() + refls.size())
    {
        va[2 * i] = sf::Vertex(refls[i - rays.size()].a, sf::Color(100u, 100u, 255u, 128u));
        va[2 * i + 1] = sf::Vertex(refls[i - rays.size()].b, sf::Color(100u, 100u, 255u, 128u));
        i++;
    }
}

float cr(sf::Vector2f a, sf::Vector2f b)
{
    return a.x * b.y - a.y * b.x;
}
float dot(sf::Vector2f a, sf::Vector2f b)
{
    return a.x * b.x + a.y * b.y;
}
float dist(sf::Vector2f a, sf::Vector2f b)
{
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
sf::Vector2f r90r(sf::Vector2f a)
{
    return sf::Vector2f(a.y, -a.x);
}

bool intersect(Ray ray, Reflector refl, sf::Vector2f &pos, float &d)
{
    float x = ((refl.b.x - refl.a.x) * cr(ray.dir, ray.src) + ray.dir.x * cr(refl.a, refl.b - refl.a)) / cr(ray.dir, refl.b - refl.a);
    float y = ((refl.b.y - refl.a.y) * cr(ray.dir, ray.src) + ray.dir.y * cr(refl.a, refl.b - refl.a)) / cr(ray.dir, refl.b - refl.a);

    bool c1 = (x - ray.src.x) * ray.dir.x >= 0;
    bool c2 = (y - ray.src.y) * ray.dir.y >= 0;
    bool c3 = (refl.a.x < x) && (x < refl.b.x) || (refl.a.x > x) && (x > refl.b.x) || abs(refl.b.x - x) < std::numeric_limits<double>::epsilon();
    bool c4 = (refl.a.y < y) && (y < refl.b.y) || (refl.a.y > y) && (y > refl.b.y) || abs(refl.b.y - y) < std::numeric_limits<double>::epsilon();

    bool is = c1 && c2 && c3 && c4;

    if (is)
    {
        pos.x = x;
        pos.y = y;
        d = dist(ray.src, pos);
    }

    return is;
}
void interact(std::vector<Reflector> &refls, sf::Vector2i mPos, bool &mSel, int &iSel)
{
    sf::Vector2f _mPos(mPos.x, mPos.y);

    if (!mSel)
    {
        iSel = -1;
        for (int i = 0; i < refls.size(); i++)
        {
            if (dist(refls[i].a, _mPos) < 10.f)
            {
                iSel = 2 * i;
                break;
            }
            if (dist(refls[i].b, _mPos) < 10.f)
            {
                iSel = 2 * i + 1;
                break;
            }
        }
    }

    if (iSel != -1 && mSel)
    {
        if (iSel % 2 == 0)
        {
            refls[iSel / 2].a = _mPos;
        }
        else
        {
            refls[(iSel - 1) / 2].b = _mPos;
        }
    }
}
bool tir(Ray &ray, Reflector &refl)
{
    sf::Vector2f &a = refl.a;
    sf::Vector2f &b = refl.b;

    sf::Vector2f n = r90r(b - a) / dist(a, b);

    sf::Vector2f &id = ray.dir;

    float &m = refl.rIdx;

    return 1.f - m * m * (1.f - dot(n, id) * dot(n, id)) < 0;
}
Ray refract(Ray &ray, Reflector &refl, sf::Vector2f &vMin, int &mMin, sf::Color col = sf::Color::White)
{
    sf::Vector2f &a = refl.a;
    sf::Vector2f &b = refl.b;

    sf::Vector2f n = r90r(b - a) / dist(a, b);

    sf::Vector2f &id = ray.dir;

    float &m = refl.rIdx;

    sf::Vector2f rd1 = sqrtf(1.f - m * m * (1.f - dot(n, id) * dot(n, id))) * n + m * (id - dot(n, id) * n);
    sf::Vector2f rd2 = -sqrtf(1.f - m * m * (1.f - dot(n, id) * dot(n, id))) * n + m * (id - dot(n, id) * n);

    sf::Vector2f rd = (dot(n, id) * dot(n, rd1) >= 0.f) ? rd1 : rd2;

    return Ray(vMin, rd, col, mMin);
}
Ray reflect(Ray &ray, Reflector &refl, sf::Vector2f &vMin, int &mMin, sf::Color col = sf::Color::White)
{
    sf::Vector2f &a = refl.a;
    sf::Vector2f &b = refl.b;

    sf::Vector2f n = r90r(b - a) / dist(a, b);

    sf::Vector2f &id = ray.dir;

    sf::Vector2f rd = id - 2 * dot(id, n) * n;

    return Ray(vMin, rd, col, mMin);
}
float reflectance(Ray &ray, Reflector &refl)
{
    sf::Vector2f &a = refl.a;
    sf::Vector2f &b = refl.b;

    sf::Vector2f n = r90r(b - a) / dist(a, b);

    float &m = refl.rIdx;

    sf::Vector2f &id = ray.dir;

    float r0 = (1.f - m) / (1.f + m) * (1.f - m) / (1.f + m);

    float r;
    if (dot(n, id) > 0.f)
    {
        r = r0 + (1.f - r0) * (1.f - dot(n, id)) * (1.f - dot(n, id)) * (1.f - dot(n, id)) * (1.f - dot(n, id)) * (1.f - dot(n, id));
    }
    else
    {
        r = r0 + (1.f - r0) * (1.f + dot(n, id)) * (1.f + dot(n, id)) * (1.f + dot(n, id)) * (1.f + dot(n, id)) * (1.f + dot(n, id));
    }

    return r;
}
void calculate(std::vector<Ray> &rays, std::vector<Reflector> refls)
{
    int r = 0;
    while (r < rays.size())
    {
        float d, dMin = std::numeric_limits<float>::infinity();
        sf::Vector2f v, vMin;
        int mMin = -1;

        for (int m = 0; m < refls.size(); m++)
        {
            bool is = intersect(rays[r], refls[m], v, d);
            if (is && d < dMin && rays[r].refl != m)
            {
                dMin = d;
                vMin = v;
                mMin = m;
            }
        }

        if (mMin != -1 && refls[mMin].reflCount <= 100)
        {
            rays[r].len = dMin;
            refls[mMin].reflCount++;

            float rc = reflectance(rays[r], refls[mMin]);

            rays.push_back(reflect(rays[r], refls[mMin], vMin, mMin, sf::Color(255u, 255u, 255u, (sf::Uint8)(rc * rays[r].col.a))));
            if (!tir(rays[r], refls[mMin]))
                rays.push_back(refract(rays[r], refls[mMin], vMin, mMin, sf::Color(255u, 255u, 255u, (sf::Uint8)((1.f - rc) * rays[r].col.a))));
        }

        r++;
    }
}

int main()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    auto window = sf::RenderWindow({1280u, 720u}, "Light Sim", sf::Style::Default, settings);
    window.setFramerateLimit(144);

    std::vector<Ray> rays;

    std::vector<Reflector> refls = {
        Reflector(sf::Vector2f(160.f, 40.f), sf::Vector2f(160.f, 320.f), 1.5f),
        Reflector(sf::Vector2f(240.f, 40.f), sf::Vector2f(240.f, 320.f), 1.5f),
        Reflector(sf::Vector2f(300.f, 40.f), sf::Vector2f(300.f, 320.f), 1.5f)};

    bool mSel = false;
    int iSel = -1;

    float angle = 0.f;

    bool incAngle = false;
    bool decAngle = false;

    while (window.isOpen())
    {
        for (auto event = sf::Event(); window.pollEvent(event);)
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::MouseButtonPressed:
                mSel = true;
                break;
            case sf::Event::MouseButtonReleased:
                mSel = false;
                break;
            case sf::Event::KeyPressed:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                    incAngle = true;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    decAngle = true;
                break;
            case sf::Event::KeyReleased:
                if (!sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                    incAngle = false;
                if (!sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    decAngle = false;
                break;
            default:
                break;
            }
        }

        if (incAngle)
            angle += M_PI / 180.f / 4.f;
        if (decAngle)
            angle -= M_PI / 180.f / 4.f;

        // input starting rays
        rays = {};

        for (int i = 0; i < 1; i++)
        {
            rays.push_back(Ray(sf::Vector2f(50.f, 60.f + 2.f * i), sf::Vector2f(cos(angle), sin(angle))));
        }

        interact(refls, sf::Mouse::getPosition(window), mSel, iSel);
        calculate(rays, refls);

        window.clear();

        sf::VertexArray va(sf::Lines, 2 * (rays.size() + refls.size()));
        setVertices(va, rays, refls);
        window.draw(va);

        window.display();
    }

    return 0;
}
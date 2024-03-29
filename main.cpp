#include "src/goopylib/goopylib.h"


class Brick : public gp::Rectangle {
public:
    Brick(int i, int j)
            : gp::Rectangle({i * s_Width - 450 + s_Width / 2,
                             j * s_Height + 85}, s_Width, s_Height) {

        setColor(gp::Color(255 - j * 20, 255 - j * 20, 255 - j * 20));
    }

    void hit() {
        m_Hits += 1;
        setTransparency(1 - 0.25f * m_Hits);

        if (m_Hits == 4) {
            destroy();
        }
    }

    int m_Hits = 0;

    static const int s_Width = 100;
    static const int s_Height = 55;
};

class Ball : public gp::Circle {
public:
    Ball() : gp::Circle({-250, 20}, 7) {
        setColor(gp::Color(255, 150, 150));
    }

    void update(float dt) {
        if (m_Position.x < -450) {
            setX(-450);
            m_xVel *= -1;
        }
        else if (m_Position.x > 450) {
            setX(450);
            m_xVel *= -1;
        }

        if (m_Position.y > 300) {
            setY(300);
            collide();
        }

        if (abs(m_yVel) < 5) {
            m_yVel -= 0.07;
            m_xVel += 0.07;
        }

        move(70 * m_xVel * dt, 70 * m_yVel * dt);
    }

    void collide() {
        m_yVel *= -1;
    }

    float m_xVel = 0;
    float m_yVel = 0;
};

// TODO fix error caused by inheriting from gp::Rectangle which leaves its virtual base (Quad) uninitialised
class Controller : public gp::Rectangle {
public:
    Controller() : gp::Rectangle({0, -220}, 120, 15) {
        setColor(gp::Color(255, 255, 255));
    }

    void updateController() {
        auto pos = m_Window->toWorld(m_Window->getMousePosition());

        float tmp = m_Position.x;
        setX(max(-m_MaxX, min(m_MaxX, pos.x)));
        m_xVel = m_Position.x - tmp;
    }

    float m_xVel = 0;
    float m_MaxX = 450;
};


void setupWindow(gp::Window& window) {
    window.setBackground(gp::Color(25, 25, 25));
    window.setResizable(true);

    window.setMinSize(600, 400);
    window.setAspectRatio(900, 600);

    auto vignette = gp::Image("assets/vignette.png", {0, 0}, 900, 600);
    vignette.draw(window);
    vignette.setTransparency(0.3, 0.3, 0.8, 0.8);
}


void setupGame(gp::Window& window, Controller& controller, Ball& ball, std::vector <Brick>& bricks) {
    controller = Controller();
    controller.draw(window);

    ball = Ball();
    ball.draw(window);

    bricks.clear();
    for (int i = 0; i < 900 / Brick::s_Width; i++) {
        for (int j = 0; j < 4; j++) {
            bricks.emplace_back(i, j);
            bricks.back().draw(window);
        }
    }
}

bool checkGameover(Ball& ball, std::vector <Brick>& bricks) {
    return ball.getY() < -300 or bricks.size() == 0;
}

void doControllerCollision(Controller& controller, Ball& ball) {
    if (controller.contains(ball.getPosition())) {
        ball.collide();
        ball.m_xVel = max(-12.0f, min(12.0f, ball.m_xVel * 0.85f + controller.m_xVel * 0.2f));
        ball.move(0, 12);
    }
}

void doBrickCollision(std::vector <Brick>& bricks, Ball& ball, float& lasthit) {
    for (auto &brick: bricks) {
        if (brick.isDrawn() and brick.contains(ball.getPosition())) {
            ball.collide();
            brick.hit();
            lasthit = gp::getTime();
            return;
        }
    }
}

void shakeCamera(gp::Window& window, float lasthit) {
    auto &camera = window.getCamera();

    if (gp::getTime() - lasthit < 0.2) {
        camera.setX(rand() % 8 - 4);
    }
    else {
        camera.setX(0);
    }
}

void waitForLeftPress(gp::Window& window, Controller& controller) {
    while (window.isOpen() and !window.checkLeftClick()) {
        controller.updateController();
        gp::update();
    }

    while (window.isOpen() and window.checkLeftClick()) {
        controller.updateController();
        gp::update();
    }
}


void introAnimation(gp::Window& window, Controller& controller, Ball& ball) {
    auto &camera = window.getCamera();
    camera.setRotation(0);
    camera.setZoom(1);

    float transparency = 0;

    while (window.isOpen() and !window.checkLeftClick()) {
        if (transparency < 0.99) {
            transparency += 0.02;
            ball.setTransparency(transparency);
        }

        controller.updateController();
        gp::update();
    }

    ball.setTransparency(1);
}

void gameoverAnimation(gp::Window& window, Controller& controller, Ball& ball) {
    auto &camera = window.getCamera();

    ball.destroy();
    auto start = gp::getTime();
    auto ease = gp::easeBounceOut(4, 0.8);
    float easet = 0.7;

    controller.m_MaxX = 350;

    while (window.isOpen() and gp::getTime() - start < easet) {
        float val = ease(gp::getTime() - start);
        camera.setRotation(4 * val);
        camera.setZoom(1 - 0.02 * val);

        controller.updateController();
        gp::update();
    }
}

void resetAnimation(gp::Window& window, Controller& controller, Ball& ball, std::vector <Brick>& bricks) {
    auto &camera = window.getCamera();

    while (window.isOpen() and camera.getRotation() > 0) {
        camera.rotate(-0.2);
        camera.setZoom(1 - 0.005 * camera.getRotation());

        controller.updateController();
        gp::update();
    }

    controller.destroy();
    for (auto &brick: bricks) {
        brick.destroy();
    }
}


int main(int argc, char *argv[]) {
    gp::init();

    gp::Window window = {900, 600, "Brick Breaker using goopylib!"};
    setupWindow(window);
    
    auto controller = Controller();
    auto ball = Ball();
    std::vector <Brick> bricks;

    while (window.isOpen()) {
        setupGame(window, controller, ball, bricks);
        introAnimation(window, controller, ball);

        float lasthit = 0;

        while (window.isOpen() and !checkGameover(ball, bricks)) {
            float start = gp::getTime();
            gp::update();

            doControllerCollision(controller, ball);
            doBrickCollision(bricks, ball, lasthit);
            shakeCamera(window, lasthit);

            controller.updateController();
            ball.update(gp::getTime() - start);
        }

        gameoverAnimation(window, controller, ball);
        waitForLeftPress(window, controller);
        resetAnimation(window, controller, ball, bricks);
    }

    gp::terminate();
    return 0;
}

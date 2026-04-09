#include <SFML/Graphics.hpp>

int main()
{
    // SFML의 렌더링 창을 생성한다.
    // 960x720 크기의 창을 열고, 제목 표시줄에는 "Tetris SFML"을 표시한다.
    sf::RenderWindow window(sf::VideoMode({960u, 720u}), "Tetris SFML");

    // 프레임 속도를 60 FPS로 제한해서 화면 갱신이 너무 빠르게 돌지 않도록 한다.
    window.setFramerateLimit(60);

    // 창이 열려 있는 동안 게임 루프를 계속 반복한다.
    while (window.isOpen())
    {
        // 창에서 발생한 이벤트(닫기, 키 입력, 마우스 입력 등)를 하나씩 꺼내서 처리한다.
        while (const std::optional event = window.pollEvent())
        {
            // 사용자가 창의 닫기 버튼(X)을 누른 경우 창을 닫는다.
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        // 이전 프레임의 화면을 지우고 배경색으로 채운다.
        window.clear(sf::Color(18, 18, 24));

        // 여기 사이에 블록, 보드, 텍스트 같은 게임 요소를 그리게 된다.

        // 이번 프레임에 그린 결과를 실제 화면에 출력한다.
        window.display();
    }

    return 0;
}

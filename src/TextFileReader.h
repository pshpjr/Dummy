#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <Types.h>
class TextFileReader {
public:
    // 생성자: 파일명을 받아서 파일을 읽고 내부 벡터에 저장
    TextFileReader(const String& filename) {
        std::wifstream file(filename);
        file.imbue(std::locale("ko_KR.UTF-8"));
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file");
        }

        String line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        file.close();

        // 난수 생성기를 위한 시드 설정
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    // 저장된 텍스트 중 무작위로 하나를 const 참조로 반환
    const String& GetRandString() const {
        if (lines.empty()) {
            throw std::out_of_range("No lines available");
        }
        int randomIndex = std::rand() % lines.size();
        return lines[randomIndex];
    }

private:
    std::vector<String> lines; // 파일에서 읽어온 텍스트를 저장할 벡터
};


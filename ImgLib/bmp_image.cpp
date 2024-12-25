#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bfType;      // Должно быть "BM"
    uint32_t bfSize;      // Общий размер файла в байтах
    uint32_t bfReserved; // Зарезервировано; должно быть 0
    
    uint32_t bfOffBits;   // Смещение до начала
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    // поля заголовка Bitmap Info Header
    uint32_t biSize;          // Размер этого заголовка
    int32_t  biWidth;         // Ширина изображения в пикселях
    int32_t  biHeight;        // Высота изображения в пикселях
    uint16_t biPlanes;        // Количество плоскостей; должно быть 1
    uint16_t biBitCount;      // Количество бит на пиксель
    uint32_t biCompression;   // Тип сжатия (0 - не сжато)
    uint32_t biSizeImage;     // Размер данных изображения в байтах
    int32_t  biXPelsPerMeter;  // Горизонтальное разрешение в пикселях на метр
    int32_t  biYPelsPerMeter;  // Вертикальное разрешение в пикселях на метр
    uint32_t biClrUsed;       // Количество используемых цветов; 0 - использовать все
    uint32_t biClrImportant;   // Количество важных цветов; 0 - все
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image){
    
    const int w = image.GetWidth();
    const int h = image.GetHeight();
    const int step = image.GetStep();
    
    BitmapFileHeader bmpFileHeader;
    
    bmpFileHeader.bfType = (static_cast<uint16_t>('M') << 8) | static_cast<uint16_t>('B');     
    bmpFileHeader.bfSize = h * GetBMPStride(w) + 54;      
    bmpFileHeader.bfReserved = 0;    
    bmpFileHeader.bfOffBits = 54; 
   
    BitmapInfoHeader bmpInfoHeader;
    
    bmpInfoHeader.biSize = 40;
    bmpInfoHeader.biWidth = w;
    bmpInfoHeader.biHeight = h;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = 24;
    bmpInfoHeader.biCompression = 0;
    bmpInfoHeader.biSizeImage =  h * GetBMPStride(w);
    bmpInfoHeader.biXPelsPerMeter = 11811;
    bmpInfoHeader.biYPelsPerMeter = 11811;
    bmpInfoHeader.biClrUsed = 0;
    bmpInfoHeader.biClrImportant = 0x1000000;
    
    ofstream out(file, ios::binary);
    out.write(reinterpret_cast<const char*>(&bmpFileHeader), sizeof(bmpFileHeader));
    out.write(reinterpret_cast<const char*>(&bmpInfoHeader), sizeof(bmpInfoHeader));
   
    std::vector<char> buff(w * 3);
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buff.data(), w * 3);
        
        for (int padding = 0; padding < (GetBMPStride(w) - w * 3); ++padding) {
            out.put(0); // Добавление нуля в качестве дополнения
        }
    }
    return out.good();
}


// напишите эту функцию
Image LoadBMP(const Path& file){
    
   ifstream ifs(file, std::ios::binary);
   if (!ifs) {
       return {};
   }
    
    BitmapFileHeader bfh;
    BitmapInfoHeader bih;
    
    ifs.read(reinterpret_cast<char*>(&bfh), sizeof(bfh));
    ifs.read(reinterpret_cast<char*>(&bih), sizeof(bih)); 
    
    // Проверка bfh
    if (bfh.bfType != 0x4D42) { // 'BM' в шестнадцатиричном представлении
        //"Неверный тип файла BMP."
        return {};
    }
    
    streampos currentPos = ifs.tellg(); // Сохраняем положение в файле
    ifs.seekg(0, istream::end);          // Переходим в конец файла
    streampos fileSize = ifs.tellg();    // Получаем размер файла
    ifs.seekg(currentPos);                     // Возвращаемся на исходную позицию
    if (bfh.bfSize != fileSize) {
        // "Размер файла не совпадает с указанным в заголовке." 
        return {};
    }
    
    if (bfh.bfOffBits < sizeof(bfh) + sizeof(bih) || bfh.bfOffBits > bfh.bfSize) {
        //"Неверное смещение данных изображения."
        return {};
    }
    // Проверка bih
    if (bih.biBitCount != 24) {
       //"Неподдерживаемый формат цвета."
        return {};
    }
    if (bih.biWidth <= 0 || bih.biHeight <= 0) {
        // "Неверные размеры изображения." 
        return {};
    }
    // Конец проверки 
    
  
    int w = bih.biWidth;
    int h = bih.biHeight;
    int padding = (4 - (w * 3) % 4) % 4;
    
    Image result(w, h, Color::Black());
    std::vector<char> buff(w * 3);

    for (int y = h-1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), w * 3);
        ifs.ignore(padding);
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
    
}

}  // namespace img_lib
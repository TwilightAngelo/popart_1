// popart_1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace cv;

// Класс инпут для загрузки изображения из файла в Mat
class Input
{
public:
	Input(string path);
	Mat GetImage();
	~Input();

private:
	Mat image;
};
Input::Input(string path)
{
	Input::image = imread(path, CV_LOAD_IMAGE_COLOR);
}
Input::~Input()
{
}
Mat Input::GetImage()
{
	return Input::image;
}


// Фильтратор собственно делает прогон изображения в новую цветовую схему
class Filterer
{
public:
	Filterer(vector<vector<int>> map);
	void ImageLoader(Mat image);
	~Filterer();
	void BrightnesRebuilt();
	Mat GetImage();
private:
	vector<Vec3b> colorMap;
	Mat image;
	Mat resultImage;
};
Filterer::Filterer(vector<vector<int>> map)
{
	Vec3b value;
	for (int rows = 0; rows < (int)map.size(); rows++) // достаём из csv цветовую схему
	{
		value.val[0] = map[rows][2] + 25;
		value.val[1] = map[rows][1];
		value.val[2] = map[rows][0];
		Filterer::colorMap.push_back(value);
	}
}
void Filterer::ImageLoader(Mat image)
{
	Filterer::image = image; // получаем изображение
	Filterer::resultImage = Mat::zeros(image.size(), image.type());;
}
Mat Filterer::GetImage()
{
	return Filterer::resultImage; //возвращаем резульитирующее изображение
}
Filterer::~Filterer()
{
}
void Filterer::BrightnesRebuilt()
{
	Mat kernel = (Mat_<char>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0); //полученные из картинки данные перегоняются в цветовую схему из csv
	filter2D(Filterer::image, Filterer::image, -1, kernel);
	for (int i = 0; i < Filterer::image.rows; i++)
	{
		for (int j = 0; j < Filterer::image.cols; j++)
		{
			Vec3b current = Filterer::image.at<Vec3b>(i, j);
			double brightness = sqrt((0.299 * current.val[2] * current.val[2]) + (0.587 * current.val[1] * current.val[1]) + (0.144 * current.val[0] * current.val[0])); //результирующий цвет выбирается исходя из яркости
			if (brightness > 255) brightness = 255;
			Filterer::resultImage.at<Vec3b>(i,j) = Filterer::colorMap[brightness];
		}
	}
}

// класс для отрисвки и создания точек
class Dot
{
public:
	Dot(Mat image);
	~Dot();
	void SetCoreSize(int size);
	void BuildDotMask();
	void ApplyMask();
	Mat GetImage();

private:
	Mat image;
	Mat resultImage;
	int coreSize;
	Mat dotMask;
};
Dot::Dot(Mat image)
{
	Dot::image = image;
}
void Dot::SetCoreSize(int size)
{
	Dot::coreSize = size; //размер диаметра кругов/точек
}
void Dot::BuildDotMask()
{
	Dot::dotMask = Mat::zeros(Dot::image.size(), Dot::image.type()); //сначала создается маска из белых кругов, по которой и будет обход сзображения
	for (int i = coreSize; i < Dot::image.rows + coreSize; i+=coreSize*2)
	{
		for (int j = coreSize; j < Dot::image.cols + coreSize; j+=coreSize*2)
		{
			circle(Dot::dotMask, cv::Point(j, i), ceil(coreSize / 2), Scalar(255, 255, 255), CV_FILLED); 
		}
	}
	resize(Dot::dotMask, Dot::dotMask, Size(Dot::image.cols, Dot::image.rows), 0, 0, INTER_CUBIC);
}
void Dot::ApplyMask()
{
	Dot::image.copyTo(Dot::resultImage);
	for (int i = 0; i < Dot::image.rows; i++)
	{
		for (int j = 0; j < Dot::image.cols; j++)
		{
			if (Dot::dotMask.at<Vec3b>(i, j) == Vec3b(255, 255, 255))
			{
				Vec3b current = Dot::image.at<Vec3b>(i, j); // применение маски на изображение
				auto middle = current.val[0] + current.val[1] + current.val[2];
				current.val[0] = current.val[0] - middle;
				current.val[1] = current.val[1] > 250 ? current.val[1] : 0; //попытка сделать правдоподобно исходя из результатов
				current.val[2] = current.val[2] - middle;
				Dot::resultImage.at<Vec3b>(i, j) = current;
			}
		}
	}
}
Mat Dot::GetImage() 
{
	return Dot::resultImage; //возвращаем полученную картинку
}
Dot::~Dot()
{
}

void onDotChange(int pos, void* ud)
{
	Dot dot = *((Dot*)ud); //обработчик трэкбара для точек, перестройка маски и изображения
	dot.SetCoreSize(pos+1);
	dot.BuildDotMask();
	dot.ApplyMask();
	imshow("Display", dot.GetImage());
}

int main(int argc, char** argv)
{
	int dotSize = 3;
	ifstream inputfile(argv[2]);
	string current_line;
	vector< vector<int> > all_data;
	while (getline(inputfile, current_line)) //получение из csv данных о цветовой схеме|| по сути в csv хранится массив размером 256
	{                                        //представляющий собой градиент от желтого к синему через красный
		vector<int> values;
		stringstream temp(current_line);
		string single_value;
		while (getline(temp, single_value, ',')) {
			values.push_back(atoi(single_value.c_str()));
		}
		all_data.push_back(values);
	}

	Input input = Input(argv[1]);
	
	Filterer filterer = Filterer(all_data);
	filterer.ImageLoader(input.GetImage());
	filterer.BrightnesRebuilt();

	Dot dot = Dot(filterer.GetImage());
	dot.SetCoreSize(2);
	dot.BuildDotMask();
	dot.ApplyMask();

	namedWindow("Display", WINDOW_AUTOSIZE);
	createTrackbar("Dots", "Display", &dotSize, 50, onDotChange, &dot);
	imshow("Display", dot.GetImage());
	waitKey(0);
    return 0;
}

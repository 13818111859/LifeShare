/*******************************************************************************
//      (C) Copyright 2015 VeriSilicon.
//          All Rights Reserved	
********************************************************************************/

/*******************************************************************************
//
//  Description:
//
//  History:
//		Date		Authors			        Changes
//		2015/12/15	Xiaodong Kong			Created
********************************************************************************/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <time.h>    
#include <queue>
#include <io.h>

using namespace cv;
using namespace std;

#define kernal 16
#define unit 768 //kernal*kernal*3
#define QUEUE 60
#define length 64
struct particle
{
	char   UID[length];  //Use ID
	char   VID[length];  //Vision ID
	char   PID[length];  //Particle ID
	char   is_repeat;  
	long   time;
	double space[3];
	char   remark[length*2];
};

double life_open(char *LIFE, char *UID, FILE **life, FILE **life_index)
{
	char index[300];
	strcpy(index, LIFE);
	strcat(index, ".sha");
	*life = fopen(index, "ab+");

	strcpy(index, LIFE);
	strcat(index, ".");
	strcat(index, UID);
	strcat(index, ".idx");
	*life_index = fopen(index, "ab+");
	if (*life == NULL || *life_index == NULL) printf("Can not open file!");
	particle record;
	fseek(*life_index, 0, SEEK_END);
	long file_length = ftell(*life_index);
	if (file_length >= sizeof(particle))
	{
		fseek(*life_index, -sizeof(particle), SEEK_END);
		fread(&record, sizeof(particle), 1, *life_index);
		return atof(record.VID);
	}
	else return 0.0;
}

int life_close(FILE *life, FILE *life_index)
{
	fclose(life);
	fclose(life_index);
	return 0;
}


int life_compare(unsigned char  *data, FILE* life, particle *record, FILE *life_index)
{
	unsigned char past_life[unit];
	char UID[length];
	int same = 0;
	double count=0;	
	fseek(life, 0, SEEK_SET);

	while (!feof(life))
	{
		fread(past_life, unit, 1, life);
		fread(UID, length, 1, life);

		same = 1;
		
	    for (int i = 0; i < unit; i++)
	    {
			if (abs(data[i] - past_life[i])>30) { same = 0; break; }
	    }
		if (same)
		{
			break;
		}
		count++;
	}
   
	if (same)
	{
		record->is_repeat = 1;
		sprintf(record->PID,"%.0f", count);
		//printf("link data: count:%f\n", count);
	}
	else
	{
		record->is_repeat = 0;
		sprintf(record->PID, "%.0f", count);
		//printf("new data: count:%f\n", count);
	}
	
	return 0;
}

int life_save(unsigned char *data, FILE* life, FILE *life_index, particle *record)
{
	
	if (!record->is_repeat)
	{
		fseek(life, 0, SEEK_END);
		fwrite(data, unit, 1, life);
		fwrite(record->UID, length, 1, life);
	}
	fwrite(record, sizeof(particle),1, life_index);
	printf("Save record: UID:%s, VID:%s, PID:%s\n", record->UID, record->VID, record->PID);
	return 0;
}



int main(int argc, char** argv)
{
	VideoCapture capture, compare;	
	time_t rawtime;  
	struct tm * timeinfo;
	clock_t start, end;
	double duration;
	queue<int>que;

	if (argc > 1)
		capture.open(argv[1]); //video
	else
		capture.open(0); //camera


	if (!capture.isOpened())
	{
		std::cerr << "Cannot read video." << std::endl;
		return -1;
	}
	else
	{
		capture.set(CAP_PROP_FRAME_WIDTH, 640);
		capture.set(CAP_PROP_FRAME_HEIGHT, 480);
		printf("width:%f, height:%f\n\n", capture.get(CAP_PROP_FRAME_WIDTH), capture.get(CAP_PROP_FRAME_HEIGHT));
	}

	namedWindow("Original", WINDOW_NORMAL);
	namedWindow("Result", WINDOW_NORMAL);

	int width = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	double fps = capture.get(CV_CAP_PROP_FPS);
	if (fps < 1) fps = 30.0;
	int blank = (int)(1000 / fps);
	Mat result;
	result.create(cvSize(kernal, kernal), CV_8UC3);
	char LIFE[300] = "Life";
	FILE *life=NULL, *life_index=NULL;	
	double frame_count=0;
	char UID[length];

	printf("Input User ID:");
	gets(UID);
	double life_count=life_open(LIFE, UID, &life, &life_index);

	
	Mat frame;
	for (;;)
	{
		capture >> frame;
		if (frame.empty())	break;		
		if(argc==1) flip(frame, frame, 1);//flip for camera


		//image processing
		Mat cur = frame;
		Mat dst = cur;
		while (1) 
		{
			if (cur.cols / 2 < kernal || cur.rows / 2 < kernal) break;
	 	    pyrDown(cur, dst, Size(cur.cols / 2, cur.rows / 2));	 	  
	 	    cur = dst;
		}
		resize(dst, result, cvSize(result.cols, result.rows));
	

		//compare life
		start = clock();
		particle record;		
		life_compare((unsigned char*)result.data, life, &record, life_index);
		end = clock();
		duration = (double)(end - start) / CLOCKS_PER_SEC;
		printf("compare time: %f seconds\n", duration);

		//save life		
		strcpy(record.UID, UID);
		sprintf(record.VID, "%.0f", life_count);
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		printf("The current date/time is: %s", asctime(timeinfo));		
		record.time = rawtime;
		sprintf(record.remark, ".\\photo\\A%s.jpg", record.PID);
		life_save(result.data, life, life_index, &record);

		Mat link;
		if (record.is_repeat)
		{
			if(!access(record.remark, 0)) link=imread(record.remark);
			circle(frame, cvPoint(20, 20), 10, CV_RGB(200, 10, 10), 3);
			
		}
		else
		{
			if(!access(".\\photo", 0)) imwrite(record.remark, frame);
			link = frame;
		}

		//queue
		if (frame_count < QUEUE) que.push(record.is_repeat);
		else 
		{ 
			que.pop(); 
			que.push(record.is_repeat); 
			float sum = 0;
			int value;
			for (int j=0; j < QUEUE; j++)
			{
				value = que.front();
				que.pop();
				que.push(value);
				if (value) sum++;
			}
			printf("Boring Index:%.1f%c\n", sum*100 / QUEUE,'\%');

		}

		//show result image
		imshow("Result", result);
		//show original image
		imshow("Original", frame);
		//show link
		if(!link.empty()) imshow("Link", link);

		frame_count++;
		life_count++;
		//printf("%s\n", record.remark);
		
		int c = waitKey(blank);
		if (c == 'q' || c == 'Q' || (c & 255) == 27) return 0;
		if (c == 'c' || c == 'C') imwrite("Capture.", frame);
		
	}

	life_close(life, life_index);

	waitKey(blank);
	
	return 0;
}

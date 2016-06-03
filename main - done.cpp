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
	char   UID[length];  //User ID
	char   VID[length];  //Visual ID
	char   PID[length];  //Private ID
	char   GID[length];  //General ID
	char   is_repeat;  
	long   time[4];
	double space[4];
	char   remark[length*3];
};

double life_init(char *LIFE_SHARE, char *UID, FILE **life_share, FILE **life_private, FILE **life_private_index, bool is_server);
double life_compare_record(unsigned char  *data, particle *record, FILE* life);
double life_compare_life(FILE *life_share, FILE* life_private, FILE* life_private_index, bool is_server);
int life_close(FILE *life_share, FILE *life_private, FILE *life_private_index);
int life_save_record(unsigned char *data, particle *record, FILE* life_share, FILE* life_private, FILE *life_private_index, bool is_server);
//int life_save_record_init(unsigned char *data, particle *record, FILE* life_share, FILE* life_private, FILE *life_private_index);


double life_init(char *LIFE_SHARE, char *UID, FILE **life_share, FILE **life_private, FILE **life_private_index, bool is_server)
{
	if (is_server)
	{
		char index[300];
		strcpy(index, LIFE_SHARE);
		strcat(index, ".sha");
		*life_share = fopen(index, "ab+");

		strcpy(index, LIFE_SHARE);
		strcat(index, ".");
		strcat(index, UID);
		strcat(index, ".sha");
		if (!access(index, 0)) *life_private = fopen(index, "rb+");


		strcpy(index, LIFE_SHARE);
		strcat(index, ".");
		strcat(index, UID);
		strcat(index, ".idx");
		if (!access(index, 0)) *life_private_index = fopen(index, "rb+");

		
		//init life_share
		if (!life_share || !life_private || !life_private_index)
		{
			printf("Can not initialize share life!");
			return -1;
		}
		printf("Initialize share life!\n");

		//return the length of life_share 
		particle past_record;
		fseek(*life_share, 0, SEEK_END);
		long file_length = ftell(*life_share);
		if (file_length >= sizeof(particle))
		{
			fseek(*life_share, -sizeof(particle), SEEK_END);
			fread(&past_record, sizeof(particle), 1, *life_share);
			return (atof(past_record.GID)+1);
		}
		else return 0.0;
	}
	else//client
	{
		char index[300];
		//strcpy(index, LIFE_SHARE);
		//strcat(index, ".sha");
		//*life_share = fopen(index, "ab+");

		strcpy(index, LIFE_SHARE);
		strcat(index, ".");
		strcat(index, UID);
		strcat(index, ".sha");
		if (!access(index, 0)) *life_private = fopen(index, "rb+");
		else *life_private = fopen(index, "wb+");

		strcpy(index, LIFE_SHARE);
		strcat(index, ".");
		strcat(index, UID);
		strcat(index, ".idx");
		if (!access(index, 0)) *life_private_index = fopen(index, "rb+");
		else *life_private_index = fopen(index, "wb+");		

		//init private life
		particle past_record;
		if (*life_private == NULL || *life_private_index == NULL)
		{
			printf("Can not start personal life!"); 
			return -1;
		}

		//return the length of life_private_index 
		fseek(*life_private_index, 0, SEEK_END);
		long file_length = ftell(*life_private_index);
		if (file_length >= sizeof(particle))
		{
			fseek(*life_private_index, -sizeof(particle), SEEK_END);
			fread(&past_record, sizeof(particle), 1, *life_private_index);
			return (atof(past_record.VID)+1);
		}
		else return 0.0;
	}
}

int life_close(FILE *life_share, FILE *life_private, FILE *life_private_index)
{
	if(life_share) fclose(life_share);
	if(life_private) fclose(life_private);
	if(life_private_index) fclose(life_private_index);
	return 0;
}


double life_compare_record(unsigned char  *data, particle *record, FILE* life, bool is_server)
{
	unsigned char past_life[unit];
	particle past_record;
	int same = 0;
	double total = 0;
	double contribute = 0;

	fseek(life, 0, SEEK_SET);

	while (!feof(life))
	{
		fread(past_life, unit, 1, life);
		fread(&past_record, sizeof(particle), 1, life);		

		same = 1;

		if (!strcmp(past_record.UID, record->UID)) contribute++;
		
	    for (int i = 0; i < unit; i++)
	    {
			if (abs(data[i] - past_life[i])>30) { same = 0; break; }
	    }
		if (same)
		{
			break;
		}
		total++;
	}
   
	if (same)
	{
		record->is_repeat = 1;
	    strcpy(record->PID, past_record.PID);
		strcpy(record->GID, past_record.GID);			

		return 0;
	}
	else
	{
		record->is_repeat = 0;
		if (is_server)
		{
			sprintf(record->GID, "%.0f", total-1);
		}
		else
		{
			sprintf(record->PID, "%.0f", total-1);
			sprintf(record->GID, "%s", "\0");
		}
		return contribute;
	}
}

double life_compare_life(FILE *life_share, FILE* life_private, FILE* life_private_index, bool is_server)
{
	unsigned char past_life[unit];
	particle past_record, index_record;
	char str[300];
	if (!life_private || !life_share ||!life_private_index)
	{
		printf("Can not compare life!\n");
		return 0;
	}
	fseek(life_share, 0, SEEK_SET);
	fseek(life_private, 0, SEEK_SET);
	fseek(life_private_index, 0, SEEK_SET);

    //update life_share in server 
	if (is_server)
	{
		while (!feof(life_private))
		{
			//collect life
			fread(past_life, unit, 1, life_private);
			fread(&past_record, sizeof(particle), 1, life_private);
			if (strcmp(past_record.GID, "\0")) continue;

			//compare life
			life_compare_record(past_life, &past_record, life_share, is_server);


			//save life		
			life_save_record(past_life, &past_record, life_share, life_private,  life_private_index, is_server);
		
		}
	}
	

	return 0;
}

int life_save_record(unsigned char *data, particle *record, FILE* life_share, FILE* life_private, FILE *life_private_index, bool is_server)
{
	if (is_server)
	{
		//save data when it is a new one
		if (!record->is_repeat)
		{
			fseek(life_share, 0, SEEK_END);
			fwrite(data, unit, 1, life_share);
			fwrite(record, sizeof(particle), 1, life_share);	
			printf("Save a share record!\n");

	    }

		//always change the GID of private
		fseek(life_private, -sizeof(particle), SEEK_CUR);
		fwrite(record, sizeof(particle), 1, life_private);
		fseek(life_private,0, SEEK_CUR);

		//always change the GID of private index record 
		particle past_record;
		long i = (long)(atof(record->VID));
		fseek(life_private_index, i*sizeof(particle), SEEK_SET);
		fread(&past_record, sizeof(particle), 1, life_private_index);
		strcpy(past_record.GID, record->GID);
		fseek(life_private_index, -sizeof(particle), SEEK_CUR);
		fwrite(&past_record, sizeof(particle), 1, life_private_index);

	
		printf("Change the private record: UID:%s, VID:%s, PID:%s, GID:%s\n", record->UID, record->VID, record->PID, record->GID);
		return 0;
	}
	else
	{
		//save data when it is a new one
		if (!record->is_repeat)
		{
			fseek(life_private, 0, SEEK_END);
			fwrite(data, unit, 1, life_private);
			fwrite(record, sizeof(particle), 1, life_private);
		}

		//always save record 
		fseek(life_private_index, 0, SEEK_END);
		fwrite(record, sizeof(particle), 1, life_private_index);
		printf("Save a record: UID:%s, VID:%s, PID:%s, GID:%s\n", record->UID, record->VID, record->PID, record->GID);
		return 0;
	}
}

//int life_save_record_init(unsigned char *data, particle *record, FILE* life_share, FILE* life_private, FILE *life_private_index)
//{
//	
//	//save data when it is a new one
//	if (!record->is_repeat)
//	{
//		fseek(life_share, 0, SEEK_END);
//		fwrite(data, unit, 1, life_share);
//		fwrite(record, sizeof(particle), 1, life_share);
//
//		particle index_record;
//		fseek(life_private, 0, SEEK_SET);
//		while (!feof(life_private_index))
//		{
//			fread(&index_record, sizeof(particle), 1, life_private_index);
//			if (!strcmp(record->VID, index_record.VID))
//			{
//				strcpy(index_record.PID, record->PID);
//				fseek(life_private_index, -sizeof(particle), SEEK_CUR);
//				fwrite(record, sizeof(particle), 1, life_private_index);
//				break;
//			}
//		}
//
//	}
//	//always save index for private		
//	else
//	{
//		particle index_record;
//		fseek(life_private, 0, SEEK_SET);
//		while (!feof(life_private_index))
//		{
//			fread(&index_record, sizeof(particle), 1, life_private_index);
//			if (!strcmp(record->VID, index_record.VID))
//			{
//				strcpy(index_record.PID, record->PID);
//				index_record.is_repeat = 1;
//				fseek(life_private_index, -sizeof(particle), SEEK_CUR);
//				fwrite(record, sizeof(particle), 1, life_private_index);
//				break;
//			}
//		}
//
//	}
//
//	printf("Save a record: UID:%s, VID:%s, PID:%s\n", record->UID, record->VID, record->PID);
//	return 0;
//	
//}



int main(int argc, char** argv)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//init
	VideoCapture capture;	
	time_t rawtime;  
	struct tm * timeinfo;
	clock_t start, end;
	double duration;
	queue<int>que;
	FILE *life_share = NULL, *life_private= NULL, *life_private_index = NULL;
	char LIFE_SHARE[300] = "LifeShare";	
	if (argc<3) return 0;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//server
	if (!strcmp(argv[1], "s"))
	{
		life_init(LIFE_SHARE, argv[2], &life_share, &life_private, &life_private_index, 1);
		life_compare_life(life_share, life_private, life_private_index, 1);
		life_close(life_share, life_private, life_private_index);

		return 0;
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//client
	if (!strcmp(argv[1], "c"))
	{
		if (argc > 3)
			capture.open(argv[3]); //video
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
		double frame_count = 0;
		double contribute = 0;

		//life init and orgnize
		printf("User ID:%s\n", argv[2]);
		double life_count = life_init(LIFE_SHARE, argv[2], &life_share, &life_private, &life_private_index, 0);

		
		//life collect, compare, and save
		Mat frame;
		for (;;)
		{
			//life collect
			capture >> frame;
			if (frame.empty())	break;
			if (argc == 3) flip(frame, frame, 1);//flip for camera


			//life compare
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
			contribute = life_compare_record((unsigned char*)result.data, &record, life_private, 0);
			end = clock();
			duration = (double)(end - start) / CLOCKS_PER_SEC;
			printf("compare time: %f seconds\n", duration);

			//save life		
			strcpy(record.UID, argv[2]);
			sprintf(record.VID, "%.0f", life_count);
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			printf("The current date/time is: %s", asctime(timeinfo));
			record.time[0] = rawtime;
			sprintf(record.remark, ".\\photo\\A%s.jpg", record.PID);
			life_save_record(result.data, &record, life_share, life_private, life_private_index, 0);


			//analyzing
			//boring index
			Mat link;
			if (record.is_repeat)
			{
				if (!access(record.remark, 0)) link = imread(record.remark);
				circle(frame, cvPoint(20, 20), 10, CV_RGB(200, 10, 10), 3);

			}
			else
			{
				if (!access(".\\photo", 0)) imwrite(record.remark, frame);
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
				for (int j = 0; j < QUEUE; j++)
				{
					value = que.front();
					que.pop();
					que.push(value);
					if (value) sum++;
				}
				printf("Boring Index:%.1f%c\n", sum * 100 / QUEUE, '\%');

			}
			//contribute
			if (!record.is_repeat)	printf("User:%s, Contribute:%.0f, Percent:%.6f%c\n", record.UID, contribute, contribute * 100 / atof(record.PID), '\%');


			//show
			//show result image
			imshow("Result", result);
			//show original image
			imshow("Original", frame);
			//show link
			if (!link.empty()) imshow("Link", link);

			//loop
			frame_count++;
			life_count++;
			////printf("%s\n", record.remark);


			//comman
			int c = waitKey(blank);
			if (c == 'q' || c == 'Q' || (c & 255) == 27) break;
			if (c == 'c' || c == 'C') imwrite("Capture.", frame);
		}

		life_close(life_share, life_private, life_private_index);


		return 0;
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
}

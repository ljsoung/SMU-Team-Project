﻿#define _WINSOCK_DEPRECATED_NO_WARNINGS  // 비주얼스튜디오 환경에서 개발 시 소켓 관련 경고 무시용
#define _CRT_SECURE_NO_WARNINGS  // 비주얼스튜디오 환경에서 개발 시 입력 관련 경고 무시용
#pragma comment(lib, "ws2_32")  // 비주얼스튜디오 환경에서 개발 시 소켓 라이브러리 지정용

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <Windows.h>
#include <string.h>

#define BUF_SIZE 500
#define BOX_SIZE 500

typedef struct recommendSave { // 추천 행사 저장용 연결리스트 
    char boxSave[BOX_SIZE];
    char recommendEventSave[BOX_SIZE];
    struct recommendSave* next;
} SAVE;

SAVE* saveList; // 추천 행사 저장용 포인터 선언
SAVE* saveData; // 추천 행사 저장용 포인터 선언

unsigned WINAPI SendMsg(void* arg);
unsigned WINAPI RecvMsg(void* arg);
char* Option1(char* msg);
char* Option2(char* msg, SOCKET sock);
char* Option3(char* msg);
void recommendEventData(SAVE* saveData); // 추천 행사 데이터
void menu(); // 메뉴판
char* detail(char* msg);
bool op2run;

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN serverAddr;
    HANDLE sendThread, recvThread, recommendThread;

    char serverIp[BUF_SIZE];

    printf("Input server IP: ");
    gets_s(serverIp, sizeof(serverIp));

    int portNum = 55557;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_STREAM, 0); // 소켓을 하나 생성한다.

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);
    serverAddr.sin_port = htons(portNum);

    connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

    // 동적 메모리 할당을 main 함수 내에서 수행
    saveList = (SAVE*)malloc(sizeof(SAVE)); // 추천 행사 저장
    saveData = saveList; // 추천 행사 저장

    sendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&sock, 0, NULL); // 메시지 전송용 쓰레드가 실행된다.
    recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&sock, 0, NULL); // 메시지 수신용 쓰레드가 실행된다.

    WaitForSingleObject(sendThread, INFINITE); // 전송용 쓰레드가 중지될 때까지 기다린다.
    WaitForSingleObject(recvThread, INFINITE); // 수신용 쓰레드가 중지될 때까지 기다린다.

    // 클라이언트가 종료를 시도한다면 이 줄 아래가 실행된다.

    closesocket(sock); // 소켓을 종료한다.ㄱ층
    WSACleanup(); // 윈도우 소켓 사용 중지를 운영체제에 알린다.

    // 메모리 해제
    while (saveData != NULL) {
        SAVE* temp = saveData;
        saveData = saveData->next;
        free(temp);
    }

    return 0;
}

unsigned WINAPI SendMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg); // 서버용 소켓을 전달한다.
    char msg[BUF_SIZE] = ""; // 검색 변수
    int option; // 옵션 선택 변수
    while (1) {
        scanf_s("%d", &option);
        if (option == 1) { // Searching_Info 검색
            Option1(msg);
            if (strcmp(msg, "n") == 0) {
                recommendEventData(saveData);
                menu();
            }
            else if (strcmp(msg, "y") == 0) {
                break;
            }
            else {
                send(sock, msg, strlen(msg), 0);
            }
        }
        else if (option == 2) { // Detail_Info
            Option2(msg, sock);
            if (strcmp(msg, "n") == 0) {
                recommendEventData(saveData);
                menu();
            }
            else if (strcmp(msg, "y") == 0) {
                break;
            }
        }
        else if (option == 3) {
            Option3(msg);
            if (strcmp(msg, "n") == 0) {
                recommendEventData(saveData);
                menu();
            }
            else {
                break;
            }
        }
    }
    return 0;
}

unsigned WINAPI RecvMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg); // 서버용 소켓을 전달한다.
    char msg[BUF_SIZE] = "";
    int strLen;
    int recommendNumber = 1;

    printf("===반갑습니다! 행사 정보 검색 시스템입니다!===\n");
    printf("===========!!!금주 행사 추천!!!===============\n");
    printf("\n");
    menu();
    while (recommendNumber <= 3) { // 반복
        strLen = recv(sock, msg, BUF_SIZE - 1, 0); // 서버로부터 메시지를 수신한다.
        if (strLen == -1) {
            return -1;
        }
        msg[strLen] = '\0'; // 문자열의 끝을 알리기 위해 설정

        // 문자열 분할

        char* eventName = strtok(msg, "|");
        char* eventRegion = strtok(NULL, "|");
        char* eventStartDate = strtok(NULL, "|");
        char* eventEndDate = strtok(NULL, "|");

        // recommendEvent 문자열의 길이 계산
        int len = snprintf(NULL, 0, "|    %s    |    %s    |    %s    |    %s    |\n", eventName, eventRegion, eventStartDate, eventEndDate);

        // recommendEvent 문자열을 동적으로 생성
        char* recommendEvent = (char*)malloc(len + 1);
        snprintf(recommendEvent, len + 1, "|    %s    |    %s    |    %s    |    %s    |\n", eventName, eventRegion, eventStartDate, eventEndDate);

        // 상단과 하단 경계선 생성
        int box_len = len;
        char* box = (char*)malloc(box_len + 1);
        memset(box, '-', box_len);
        box[box_len] = '\0';

        // 금주 추천 행사 알림표 출력
        printf("%s\n", box);
        printf("%s\n", recommendEvent); // 서버로부터 수신한 메시지 출력
        printf("%s\n\n", box);

        // 추천 행사 저장
        strcpy(saveList->boxSave, box);
        strcpy(saveList->recommendEventSave, recommendEvent);
        saveList->next = (SAVE*)malloc(sizeof(SAVE));
        saveList = saveList->next;

        // 메모리 해제
        free(box);
        free(recommendEvent);
        recommendNumber++;
    }
    saveList->next = NULL;

    menu(); // 추천 후 메뉴 띄우기

    while (1) { // 반복
        strLen = recv(sock, msg, BUF_SIZE - 1, 0); // 서버로부터 메시지를 수신한다.
        if (strLen == -1) {
            return -1;
        }

        msg[strLen] = '\0'; // 문자열의 끝을 알리기 위해 설정
        if (op2run == true) {
            fputs(msg, stdout); // 수신한 메시지를 표준 출력으로 출력
        }
        printf("\n");   // 데이터 구분을 위한 줄넘김(6.20 추가)
        /*
        // 여기서부터 필요 없어 보입니다.
        // detail 정보 요청 시 세부 사항 출력
        if (strLen > 0 && strcmp(msg, "detail") == 0) { // 수신한 메시지가 "detail" 인 경우
            printf("Do you want more detail information? (y/n): ");
            fgets(msg, BUF_SIZE, stdin);    // 사용자로부터 y/n 입력 받기
            msg[strcspn(msg, "\n")] = '\0'; // 입력 받은 메시지의 개행 문자 제거

            if (!strcmp(msg, "y")) {
                send(sock, "detail", strlen("detail"), 0); // 서버로 "detail" 메시지 전송
            }
        }
        else if (strLen > 0 && strcmp(msg, "end") == 0) {
            printf("Returning to the event list.\n");
        }
        else {
            printf("Event Detail: %s\n", msg); // 수신한 세부 사항 출력
        }*/
    }
    return 0;
}

char* Option1(char* confirm) {
    op2run = false;
    char dateOrLocationOrEnd[BUF_SIZE] = "";
    printf("(Date / Location) 검색 기준을 입력해주세요.\n"); // Date 또는 Location
    printf("종료를 원하시면 End를 입력해주세요.\n");
    getchar();
    gets_s(dateOrLocationOrEnd, sizeof(dateOrLocationOrEnd));
    while (1) {
        if (strcmp(dateOrLocationOrEnd, "Date") == 0) { // Date를 입력할 시
            while (1) { // 올바르게 입력할 때까지 반복
                printf("(Today / Month) 날짜 검색 기준을 입력해주세요. (대소문자 주의)\n");
                gets_s(confirm, sizeof(confirm)); // 입력을 받는다.
                if (strcmp(confirm, "Today") != 0 && strcmp(confirm, "Month") != 0) {
                    printf("올바르게 입력해주세요.\n");
                }
                else {
                    op2run = true;
                    break; // 올바르게 입력하면 break
                }
            }
            return confirm;
        }
        else if (strcmp(dateOrLocationOrEnd, "Location") == 0) { // Location를 입력할 시
            while (1) { // 올바르게 입력할 때까지 반복
                printf("(GG / Seoul / InCheon) 지역 검색 기준을 입력해주세요. (대소문자 구분)\n");
                gets_s(confirm, sizeof(confirm));
                if (strcmp(confirm, "GG") == 0 || strcmp(confirm, "Seoul") == 0 || strcmp(confirm, "InCheon") == 0) {
                    op2run = true;
                    break; // 올바르게 입력하면 break
                }
                else {
                    printf("올바르게 입력해주세요.\n");
                }
            }
            return confirm;
        }
        else if (strcmp(dateOrLocationOrEnd, "End") == 0) { // End를 입력할 시
            while (1) {
                printf("메인 페이지로 돌아가시려면 'n'\n");
                printf(" 프로그램 종료를 원하시면 'y'를 누르세요\n");
                gets_s(confirm, sizeof(confirm));
                if (strcmp(confirm, "n") != 0 && strcmp(confirm, "y") != 0) {
                    printf("올바르게 입력해주세요.\n");
                }
                else {
                    break; // 올바르게 입력하면 break
                }
            }
            return confirm;
        }
        else {
            printf("올바르게 입력해주세요.\n");
        }
    }
}

char* Option2(char* confirm, SOCKET sock) {
    char eventName[BUF_SIZE];  // 입력 받을 문자열 버퍼
    
    op2run = true;
    printf("행사 이름을 입력하세요: ");
    fscanf_s(stdin, "%s", eventName, sizeof(eventName));  // gets가 제대로 작동 안하는 관계로 수정 6.20

    // 입력 받은 행사 이름을 서버로 전송
    send(sock, eventName, strlen(eventName), 0);
    // 서버로부터 세부 정보 수신

    // 다시 명령어를 수행할 것인지 메뉴로 돌아갈 것인지 묻기
    while(1) {
        Sleep(100);
        printf("기능을 다시 사용하려면 'y'\n");
        printf("메뉴로 돌아가려면 'n'을 입력하세요\n");
        getchar();
        gets_s(confirm, BUF_SIZE);
        if (strcmp(confirm, "y") == 0) {
            Option2(confirm, sock); // Option2 함수를 다시 호출하여 새로운 요청 처리
            break;
        }
        else if (strcmp(confirm, "n") == 0) {
            recommendEventData(saveData); // 추천 행사 데이터를 출력
            menu(); // 메뉴를 출력
            break;
        }
        else {
            printf("올바르게 입력해주세요.\n");
        }
    }
    return 0;
}

char* Option3(char* confirm) {
    while (1) {
        printf("메인 페이지로 돌아가려면 'n'\n");
        printf("프로그램 종료를 원하시면 'y'를 입력하세요\n");

        // 사용자 입력 받기
        if (scanf("%s", confirm) != 1) {    //gets랑 break도 fscanf와 exit로 교체했습니다.
            // 입력 오류 처리
            fprintf(stderr, "입력 오류가 발생하였습니다.\n");
            return NULL;
        }

        // 입력값 검증 및 처리
        if (strcmp(confirm, "n") != 0 && strcmp(confirm, "y") != 0) {
            printf("올바르게 입력해주세요.\n");
        }
        else {
            break; // 올바르게 입력하면 반복문 탈출
        }
    }
    return confirm;
}

void recommendEventData(SAVE* data) { // 추천 행사 데이터
    printf("===반갑습니다! 행사 정보 검색 시스템입니다!===\n");
    printf("===========!!!금주 행사 추천!!!===============\n");
    SAVE* current = data;
    while (current->next != NULL) {
        printf("%s\n", current->boxSave);
        printf("%s\n", current->recommendEventSave);
        printf("%s\n\n", current->boxSave);
        current = current->next;
    }
}

void menu() { // 메뉴판
    printf("원하시는 옵션의 숫자를 입력해주세요.\n");
    printf("--------------------------\n");
    printf("|                        |\n");
    //printf("|                        |\n");
    printf("|   1. 행사 정보         |\n");
    printf("|                        |\n");
    printf("|   2. 세부 사항         |\n");
    printf("|                        |\n");
    printf("|   3. 프로그램 종료     |\n");
    printf("|                        |\n");
    //printf("|                        |\n");
    printf("--------------------------\n");
    printf("\n");
}

char* detail(char* confirm) {
    while (1) {
        printf("Do you want more detail information? (y/n): ");
        fgets(confirm, BUF_SIZE, stdin);               // 사용자로부터 y/n 입력 받기
        confirm[strcspn(confirm, "\n")] = '\0';        // 입력 받은 확인 메시지의 개행 문자 제거

        if (!strcmp(confirm, "y")) {
            break;
        }
        else if (!strcmp(confirm, "n")) {
            break; // detail 요청 하지 않음 메시지, 입력 단계로 돌아감
        }
        else {
            printf("Error message. Please enter 'y' or 'n'.\n"); // 다른 문자 입력 시 오류 메시지 출력
        }
    }
    return confirm;
}
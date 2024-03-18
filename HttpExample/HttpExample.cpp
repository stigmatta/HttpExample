#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <ctime>
using namespace std;

string findValue(const string& jsonString, const string& fieldName,bool isItNumber,bool isJson) {

    int startPos = jsonString.find(fieldName);

    if (startPos != string::npos) {

        if (!isJson) {
            int colonPos = jsonString.find(":", startPos);
            int newColon = jsonString.find("\n", colonPos);

            return jsonString.substr(colonPos + 1, newColon - colonPos);
        }

        else {

            int colonPos = jsonString.find(":", startPos);
            int valueStartPos = jsonString.find_first_not_of(" \t\r\n", colonPos + 1);
            int commaPos = jsonString.find(",", valueStartPos);
            if (commaPos == string::npos) {
                commaPos = jsonString.find("}", valueStartPos);
            }
            return isItNumber ? jsonString.substr(valueStartPos, commaPos - valueStartPos - 2) : jsonString.substr(valueStartPos + 1, commaPos - valueStartPos - 2);
        }
    }

    return "";
}

string convertNumToTime(const string& timestamp) {
    time_t time = stoi(timestamp);

    struct tm* timeInfo = gmtime(&time);

    char timeString[80];
    strftime(timeString, 80, "%H:%M:%S", timeInfo);

    return string(timeString);
}

int main()
{
    setlocale(0, "ru");

    //1. инициализация "Ws2_32.dll" для текущего процесса
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {

        cout << "WSAStartup failed with error: " << err << endl;
        return 1;
    }  

    //инициализация структуры, для указания ip адреса и порта сервера с которым мы хотим соединиться
   
    char hostname[255] = "api.openweathermap.org";
    
    addrinfo* result = NULL;    
    
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo(hostname, "http", &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return 3;
    }     

    SOCKET connectSocket = INVALID_SOCKET;
    addrinfo* ptr = NULL;

    //Пробуем присоединиться к полученному адресу
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        //2. создание клиентского сокета
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

       //3. Соединяемся с сервером
        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    //4. HTTP Request

    string uri = "/data/2.5/weather?q=Odessa&appid=75f6e64d49db78658d09cb5ab201e483&mode=JSON";

    string request = "GET " + uri + " HTTP/1.1\n"; 
    request += "Host: " + string(hostname) + "\n";
    request += "Accept: */*\n";
    request += "Accept-Encoding: gzip, deflate, br\n";   
    request += "Connection: close\n";   
    request += "\n";

    //отправка сообщения
    if (send(connectSocket, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
        cout << "send failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 5;
    }

    //5. HTTP Response

    string response;

    const size_t BUFFERSIZE = 1024;
    char resBuf[BUFFERSIZE];

    int respLength;

    do {
        respLength = recv(connectSocket, resBuf, BUFFERSIZE, 0);
        if (respLength > 0) {
            response += string(resBuf).substr(0, respLength);           
        }
        else {
            cout << "recv failed: " << WSAGetLastError() << endl;
            closesocket(connectSocket);
            WSACleanup();
            return 6;
        }

    } while (respLength == BUFFERSIZE);

    string country = findValue(response, "country",false,true);
    string city = findValue(response, "name",false,true);
    string date = findValue(response, "Date",false,false);
    string coord = findValue(response, "lon",true,true);
    coord += ";";
    coord += findValue(response, "lat",true,true);
    string temperature = findValue(response, "temp", true,false);
    int celsiusTemperature = int((stoi(temperature) - 32) * (5.0 / 9.0));
    temperature = to_string(celsiusTemperature);
    temperature += " C";
    string sunrise = findValue(response, "sunrise", true,false);
    sunrise = convertNumToTime(sunrise);
    string sunset = findValue(response, "sunset", true,false);
    sunset = convertNumToTime(sunset);

    cout << "Country: " << country << endl;
    cout << "City: " << city << endl;
    cout << "Date: " << date << endl;
    cout << "Coordinates: " << coord << endl;
    cout << "Temperature: " << temperature << endl;
    cout << "Sunrise time: " << sunrise << endl;
    cout << "Sunset time: " << sunset << endl;
 
    //отключает отправку и получение сообщений сокетом
    iResult = shutdown(connectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed: " << WSAGetLastError() << endl;
        closesocket(connectSocket);
        WSACleanup();
        return 7;
    }

    closesocket(connectSocket);
    WSACleanup();
}
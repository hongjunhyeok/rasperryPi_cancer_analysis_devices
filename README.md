# rasperryPi_cancer_analysis_devices

**라즈베리파이를 이용한 자궁경부암 측정리더기**


----------------
Version Settings.

1. QT Version 4 

2. OpenCV 3.4.0

3. Ubuntu 16.04

4. Raspberry Pi 3
----------------

## Main Code Files
--------------
1. testprocess.pro 
- 프로그램 라이브러리 추가 및 profile 설정하는 파일

2. main.cpp
- QT 어플리케이션을 실행시켜 윈도우를 출력하는 파일

3. mainwindow.cpp
- 프로그램 핵심파트로 평준화알고리즘, 데이터베이스 연동, 이미지처리 및 GPIO 연동
----------------
![image](https://user-images.githubusercontent.com/34786411/105119836-72e4c180-5b14-11eb-9050-186241f1bec2.png)

수정된 사항

  -사용자편의를 위해 기존 카메라 영역을 대신해 안내 문구영역 삽입.

  -관리자/사용자 모드를 두어 관리자가 필요한 기능이 추가됨.(카메라 화면, 자세한 결과값)

  -효율적인 결과측정을 위한 보정 알고리즘 추가

![image](https://user-images.githubusercontent.com/34786411/105119955-a9224100-5b14-11eb-8976-fe8a5e4a176f.png)




# 측정결과
![image](https://user-images.githubusercontent.com/34786411/105120000-c525e280-5b14-11eb-80b4-35539d39801b.png)

동일한 환경에서 13번 밴드 측정치를 60번 실험한 결과

보정 전 : 평균 76.2 표준편차 7.11

보정 후 : 평균 76.6 표준편차 3.08

기존의 약 68%의 정확도를 95%까지 향상

# EE2405-FINAL-PROJECT-BOE-BOT-CAR
EE2405 FINAL PROJECT BOE BOT CAR

In this repo, there are 3 codes, the main.cpp, bbcar.cpp, bbcar.h

here are some additional function (rotate function) in bbcar.cpp and bbcar.h 
this is why I include the library in this repo so you can change the original one with this

To run the project :
1. install all the parts for the BOE Bot car by following this video https://365nthu.sharepoint.com/sites/EE24052022Fall/_layouts/15/stream.aspx?id=%2Fsites%2FEE24052022Fall%2FShared%20Documents%2FGeneral%2FBBcar%5Finstallation%2Emp4&ga=1
2. Pay attention to the connection
 ![image](https://user-images.githubusercontent.com/113495521/210502257-055cb1f2-6270-4f1f-b33f-65cf8242deb3.png)
3. Import bbcar library https://gitlab.larc-nthu.net/ee2405_2022/bbcar.git
4. Import PWMin library https://gitlab.larc-nthu.net/ee2405_2022/pwmin.git
5. copy the main.cpp and replace the bbcar.cpp and bbcar.h with the one in this repo
6. The map design I used is as follows :
![image](https://user-images.githubusercontent.com/113495521/210503053-7b08f27d-ec6d-4808-accf-e6e224e7a172.png)
7. Run the code and place the car in middle part of the map and on the black lines.
8. You can see that the car will follow the lines and will turn left at the roundabout, on the white marker it will
calculate whether it could pass the obstacle or not.

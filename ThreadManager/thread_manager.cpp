#include "Main/headfiles.h"
#include "ThreadManager/thread_manager.h"
#include "ArmorDetector/armor_detector.h"
#include "CameraManager/camera_manager.h"
#include "Communication/communication.h"
#include "RuneDetector/rune_detector.h"
#include <random>
//#define RECEIVE//是否接受电控数
ThreadManager::ThreadManager():
    p_armor_detector_(std::make_unique<ArmorDetector>()),
    p_forecast_(std::make_unique<Forecast>()),
    p_angle_solver_(std::make_unique<AngleSolver>()),
    p_camera_manager_(std::make_unique<CameraManager>()),
    p_communication_(std::make_unique<Communication>()),
    p_run_detector_(std::make_unique<RuneDetector>()),
    i(0),
    j(0)
{}

void ThreadManager::InitThreadManager(){
    std::string xml_path = "../xml_path/thread.xml";
    cv::FileStorage fr;
    fr.open(xml_path,cv::FileStorage::READ);
    while(!fr.isOpened()){
        std::cout << "armor_xml floading failed..." << std::endl;
        fr=cv::FileStorage(xml_path, cv::FileStorage::READ);
        fr.open(xml_path, cv::FileStorage::READ);
    }
    fr["FPS"] >> FPS_count_; // 读取fps值
    this->base_time_ = 1000 / FPS_count_; // 时间的单位是ms
}

void ThreadManager::Init(){
    this->InitThreadManager();
    p_camera_manager_ -> InitCamera();
    p_armor_detector_ -> InitArmor();
    p_communication_ -> InitCom();
    p_angle_solver_ ->InitAngle();
    p_forecast_->Init();
    kalman_recv.Kalman_init();
    p_communication_->open();
    memset(locker, false, sizeof locker);
}

void ThreadManager::Produce(){
    while(1)
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        bool IsRecv=false;
#ifndef RECEIVE
        y_p_recv[i][0]=y_p_recv[i][1]=y_p_recv[i][2]=y_p_recv[i][3]=y_p_recv[i][4]=0;
#endif
#ifdef RECEIVE
        cout<<"Recving..."<<endl;
        p_communication_->RecvMcuData(y_p_recv[i],IsRecv);
        cout<<"Recv complete"<<endl;
        if(!IsRecv)
        {
            y_p_recv[i][0]=y_p_recv[i][1]=y_p_recv[i][2]=y_p_recv[i][3]=y_p_recv[i][4]=0;

        }
#endif
        buffer[i] = p_camera_manager_ -> ReadImage();
        getSystime(sys_time[i]);
        y_p_recv[i][4]=sys_time[i];
        locker[i] = true;
        condition.notify_one(); //通知wait()函数，解除阻止
        if( (++i) % 30 == 0 )
        {
            i = 0;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        // 稳定帧率每秒100帧
        int time = 10e6 - ((static_cast<std::chrono::duration<double, std::nano>>(t2 - t1)).count());
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::nanoseconds(time);
        // 使用循环和 std::this_thread::yield 函数来让当前线程让出CPU，直到指定的时间到达为止。
        while (std::chrono::steady_clock::now() < end_time) {
            std::this_thread::yield();
        }
        auto t3 = std::chrono::high_resolution_clock::now();
        // std::cout << "ProducerFPS: " << 1000/(static_cast<std::chrono::duration<double, std::milli>>(t3 - t1)).count() << std::endl;
        std::cout << "ProducerTime: " << (static_cast<std::chrono::duration<double, std::milli>>(t2 - t1)).count() << " ms" << std::endl;

    }
}


void ThreadManager::Consume(){

    while(1)//图像处理，可根据实际需求在其中添加，仅需保证consume处理速度>communicate即可。
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        if(i == j)
        {
            std::unique_lock <std::mutex> lock(mutex);
            condition.wait(lock);
        }
        // 测试多线程锁的问题
        p_armor_detector_ -> LoadImage(buffer[j]);
        locker[j] = false;

        p_communication_->UpdateData(
                p_angle_solver_->WorldToAngle(
                    p_forecast_->WorldForcast(
                p_angle_solver_->SolveWorldPoint(
              p_armor_detector_->DetectObjectArmor(),y_p_recv[j]) , sys_time[j],y_p_recv[j][2])
                    ,y_p_recv[j]));

        p_communication_ ->shoot_err(p_angle_solver_ ->shoot_get());

        // std::promise<Point2f> shoot;

        // p_run_detector_ -> getShootAim(buffer[i], sys_time[j], shoot);
        // debug
        p_armor_detector_ -> Show();
        //p_armor_detector_ -> baocun();
        if( (++j) % 30 == 0 )
        {
            j = 0;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        //！std::cout << "ConsumerTime: " << (static_cast<std::chrono::duration<double, std::milli>>(t2 - t1)).count() << " ms" << std::endl;
        //！std::cout << "ConsumerFPS: " << 1000/((static_cast<std::chrono::duration<double, std::milli>>(t2 - t1)).count()) << std::endl;
        std::cout << "ConsumerTime: " << (static_cast<std::chrono::duration<double, std::milli>>(t2 - t1)).count() << " ms" << std::endl;
        std::cout << "ConsumerFPS: " << 1000/((static_cast<std::chrono::duration<double, std::milli>>(t2 - t1)).count()) << std::endl;
    }
}

void ThreadManager::Communicate(){ //传递信息就直接修改p_communication_->Infantry中对应的数值即可
    while(1)
    {
       // cout<<"comst"<<endl;
        p_communication_->communication(p_communication_ -> Infantry);
    }
}


#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include "Main/headfiles.h"
#include "ArmorDetector/armor_detector.h"
#include "CameraManager/camera_manager.h"
#include "Pose/angle_solver.h"
#include "Communication/communication.h"
#include "Timer/Timer.h"
#include "Forecast/forecast.h"
#include "RuneDetector/rune_detector.h"
#include "GafSolver/iterative_projectile_tool.h"
#include "GafSolver/gimbal_transform_tool.h"
#include "GafSolver/gaf_projectile_solver.h"
//extern double yaw;
//extern double pitch;



class ThreadManager
{
public:
    ThreadManager();
    void Init();
    void Produce();
    void Consume();
    void Communicate();



private:
    void InitThreadManager();


private:
    int FPS_count_;                                          // 帧数
    int base_time_;                                          // 稳定的时间
    std::unique_ptr<ArmorDetector> p_armor_detector_;        // 装甲识别
    std::unique_ptr<Forecast> p_forecast_;
    std::unique_ptr<Kalman>p_kalman_;
   //    std::unique_ptr<RuneDetector> p_rune_dectector_;    // 大符识别
    std::unique_ptr<AngleSolver> p_angle_solver_;            // 角度解算
    std::unique_ptr<CameraManager> p_camera_manager_;        // 相机读图管理
    //    std::unique_ptr<Classify> p_classify_;             // 机器学习 弃用
    std::unique_ptr<Communication> p_communication_;         //电控通信

    std::unique_ptr<RuneDetector> p_run_detector_;           //

    double y_p_recv[30][5];//yaw_angle;yaw_speed;pit_angle;pit_speed;(time)
    Mat buffer[30];
    double sys_time[30];
    bool locker[30];
    int i,j;
    std::mutex mutex;
    std::condition_variable condition; //条件变量对象
    vector<Point2d> object_armor_2Dpoints_;
    double sys_now;

    Kalman kalman_recv;//用于平滑陀螺仪数据中的pitch和yaw角度

    double yaw=0,pit=0;
    int yr=1,pr=1;


};

#endif // THREADMANAGER_H

#pragma once
#include <iostream>
using namespace std;
#include"worker.h"
//员工类
class boss :public worker
{
public:
	//构造函数，传入员工的个人信息
	boss(int id, string name, int did=3);
	//显示个人信息
	virtual void showinfo();
	//获取职工岗位名称
	virtual string getdeptname();

};

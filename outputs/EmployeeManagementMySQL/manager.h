#pragma once
#include<iostream>
using namespace std;
#include"worker.h"
//经理类
class manager :public worker
{
public:
	//构造函数，传入经理的个人信息
	manager(int id, string name, int did=2);
	//显示个人信息
	virtual void showinfo();
	//获取职工岗位名称
	virtual string getdeptname();

};
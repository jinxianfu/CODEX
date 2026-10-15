#pragma once
#include <iostream>
#include <string>
using namespace std;
//职工抽象类
class worker
{
public:
	virtual ~worker() = default;
	//显示个人信息
	virtual void showinfo()=0;
	//获取岗位名称
	virtual string getdeptname() = 0;
	int m_id;//职工编号
	string m_name;//职工姓名
	int m_deptid;//职工所在部门编号
};

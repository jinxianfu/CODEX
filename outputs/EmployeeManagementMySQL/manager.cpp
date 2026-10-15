#include "manager.h"
//传入员工信息，初始化
manager::manager(int id, string name, int did)
{
	this->m_id = id;
	this->m_name = name;
	this->m_deptid = did;//部门编号
}
//打印职工信息
void manager::showinfo()
{
	cout << "职工编号:" << this->m_id 
		<< "\t职工姓名:" << this->m_name 
		<< "\t岗位:" << this->getdeptname() 
		<< "\t岗位职责:完成老板交给的任务,并下发任务给员工" << endl;
}
string manager::getdeptname()
{
	return string("经理");
}
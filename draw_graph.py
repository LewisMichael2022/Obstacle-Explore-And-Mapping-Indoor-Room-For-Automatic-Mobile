#import sys
#sys.path.append("d:\\anaconda\lib\site-packages")c

import numpy as np
import threading
import time
import socket
import os
map_x=3
map_y=3

north_sign='🔼'
south_sign='🔽'
east_sign='▶️ '
west_sign='◀️ '
obstacle_sign='🟥'
no_obstacle_sign='  '
no_check_sign='。'
my_IP='192.168.137.1'
reveive_port=12346
target_IP='192.168.137.126'
target_port=12345

def decode_string(data):
    len=0
    i=0
    while(data[i]!='$'):
        i+=1
    length=int(data[0:i])
    i+=1
    count=0
    res=""
    while(count<length):
        res+=data[i]
        count+=1
        i+=1
    return res

def get_information():                                      #超聲波回傳障礙物 0=超聲波沒障礙物 1=超聲波回傳有障礙物 2=碰撞回傳有障礙物
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)# 绑定到本地地址和端口号
        sock.connect((target_IP, target_port))# 接收從Arduino傳送過來的資料
        data=""
        while(1):
                c=sock.recv(1)# 處理接收到的資料
                c=c.decode()
                if(c=='\n'):
                    break
                else:
                    data+=c
        sock.close()
        data=decode_string(data)
        return data

def send_instruction(string):
    string=str(len(string))+"$"+string
    # 创建套接字
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # 连接到接收方
    sock.connect((target_IP, target_port))# 发送数据
    data = string
    data+="\n"
    sock.send(data.encode())

    # 关闭套接字
    sock.close()


class Bot:
    def __init__(self):
        self.position=np.array([1,1])
        self.direction=np.array([1,0]) #facing north->(0,-1), south->(0,1), east->(1,0), west(-1,0)
	  
        self.x_size=map_x #bot地圖的x和y的大小
        self.y_size=map_y

        self.state=east_sign
        self.map=[] #bot自己繪製的地圖

        for i in range(0,self.x_size):#initialize bot.map
            self.map.append([])
            for j in range(0,self.y_size):
                self.map[i].append(0) 
		#print(str(self.x_size)+"x"+str(self.y_size))
        self.place_bot() 

    def place_bot(self):#在終端機顯示自己的狀態
        self.map[self.position[0]][self.position[1]]=self.state
	
    def remove_bot(self):#把走過的位置放2
        self.map[self.position[0]][self.position[1]]=2

    def forward(self):
        send_instruction("f")
        instruction_res=get_information()
        while(instruction_res!="0" and instruction_res!="1"):
            print("error"+instruction_res)
            send_instruction("f")
            instruction_res=get_information()  
        instruction_res=int(instruction_res)
        '''send_instruction("@3")
        test_string=get_information()
        print(test_string)'''

        if(instruction_res==0 or instruction_res==1): 
            self.remove_bot()
            self.position = self.position + self.direction
            self.place_bot()
            if(self.position[0]==self.x_size-1):
                print("bound append x")
                self.x_size+=1
                self.map.append([])
                for i in range(0,self.y_size):
                    self.map[self.x_size-1].append(0)
		
            if(self.position[0]==0):
                self.x_size+=1
                self.map.insert(0,[])
                for i in range(0,self.y_size):
                    self.map[0].append(0)
                self.position[0]+=1

            if(self.position[1]==self.y_size-1):
                print("bound append y")
                self.y_size+=1
				#self.map[self.y_size-1].append(0)
                for j in range(0,self.x_size):
                    self.map[j].append(0)
			
            if(self.position[1]==0):
                self.y_size+=1
                for j in range(0,self.x_size):
                    self.map[j].insert(0,0)
                self.position[1]+=1
			
            if(instruction_res==0):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=2
                print_map(self)
                return 0
            elif(instruction_res==1):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=1
                print_map(self)
                return 1
        elif(instruction_res==2):
            self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=1
            print_map(self)
            return 1


    # def back(self):
    #     if(self.map[self.position[0]+self.direction[0]*-1][self.position[1]+self.direction[1]*-1]==2):#?
    #         send_instruction("B")
    #         instruction_res=get_information()
    #         if(instruction_res=='error'):
    #             self.back()
    #         self.remove_bot()
    #         self.position = self.position + self.direction*-1
    #         self.place_bot()
    #         return 0
    #     else:
    #         return 1
	
    def turn_left(self):
        send_instruction("l")
        instruction_res=get_information()
        while(instruction_res!="0" and instruction_res!="1"):
            print("error"+instruction_res)
            send_instruction("l")
            instruction_res=get_information()  
        instruction_res=int(instruction_res)
        '''send_instruction("@3")
        test_string=get_information()
        print(test_string)'''

        dir=list(self.direction)
        if(dir==[0,-1]):
            self.direction=np.array([-1,0])   #🔼=>◀️
            self.state=west_sign
        elif(dir==[-1,0]):
            self.direction=np.array([0,1])    #◀️=>🔽
            self.state=south_sign
        elif(dir==[0,1]):
            self.direction=np.array([1,0])    #🔽=>▶️
            self.state=east_sign
        elif(dir==[1,0]):
            self.direction=np.array([0,-1])   #▶️=>🔼
            self.state=north_sign
        else:
            print("error")
        self.place_bot()
        if(instruction_res==0):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=2
                print_map(self)
                return 0
        elif(instruction_res==1):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=1
                print_map(self)
                return 1
                
	
    def turn_right(self):
        send_instruction("r")
        instruction_res=get_information()
        while(instruction_res!="0" and instruction_res!="1"):
            print("error"+instruction_res)
            send_instruction("r")
            instruction_res=get_information()  
        instruction_res=int(instruction_res)
        '''send_instruction("@3")
        test_string=get_information()
        print(test_string)'''

        dir=list(self.direction)
        if(dir==[0,-1]):
            self.direction=np.array([1,0])    #🔼=>▶️
            self.state=east_sign
        elif(dir==[1,0]):
            self.direction=np.array([0,1])    #▶️=>🔽
            self.state=south_sign
        elif(dir==[0,1]):
            self.direction=np.array([-1,0])   #🔽=>◀️
            self.state=west_sign
        elif(dir==[-1,0]):
            self.direction=np.array([0,-1])   #◀️=>🔼
            self.state=north_sign
        else:
            print("error")
        self.place_bot()
        if(instruction_res==0):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=2
                print_map(self)
                return 0
        elif(instruction_res==1):
                self.map[self.position[0]+ self.direction[0]][self.position[1]+ self.direction[1]]=1
                print_map(self)
                return 1

#main dish
#1 turn right ,2 turn left


def path1(path_num,ob):
    if(path_num==3):
        return 0
    while(ob==0):
        ob=bot.forward()
    ob=bot.turn_right()
    if(ob):
        ob=bot.turn_right()
        if(ob):
            ob=bot.turn_right()        #To next phase
            path_num+=1             
            path1(path_num,ob)
        else:
            path1_block(path_num)
    else:
        ob=bot.forward() #if ob=2
        if(ob==2):
            ob=bot.turn_right()
            if(ob):
                ob=bot.turn_right()    #To next phase
                path_num+=1         
                path1(path_num,ob)
            else:
                path1_block(path_num)
        else:
            ob=bot.turn_right()
            if(ob):
                path1_double_block(path_num)
            else:
                ob=bot.forward() #if ob=2
                if(ob==2):
                    path1_double_block(path_num)
                else:
                    path2(path_num,ob)


def path1_block(path_num):
    ob=bot.forward() #if ob=2
    if(ob==2):
        ob=bot.turn_right()    #To next phase
        path_num+=1         
        path1(path_num)
    else:
        ob=bot.turn_left()
        if(ob):
            ob=bot.turn_right()
            if(ob):
                ob=bot.turn_right()    #To next phase
                path_num+=1
                path1(path_num,ob)
            else:
                path1_block(path_num)
        else:
            ob=bot.forward()        #if ob=2
            if(ob==2):
                ob=bot.turn_right()
                if(ob):
                    ob=bot.turn_right()    #To next phase
                    path_num+=1         
                    path1(path_num,ob)
                else:
                    path1_block(path_num)
            else:
                ob=bot.turn_right()
                if(ob):
                    path1_double_block(path_num)
                else:
                    path2(path_num,ob)

def path1_double_block(path_num):
    ob=bot.turn_right()
    if(ob):
        print("error")
    else:
        ob=bot.forward() #if ob=2
        if(ob==2):
            print("error")
        else:
            ob=bot.turn_left()
            if(ob):
                ob=bot.turn_right()    #To next phase
                path_num+=1         
                path1(path_num,ob)
            else:
                path1_block(path_num)

def path2(path_num,ob):
    if(path_num==3):
        return 0
    while(ob==0):
        ob=bot.forward()
    ob=bot.turn_left()
    if(ob):
        ob=bot.turn_left()
        if(ob):
            bot.turn_left()
            ob=bot.turn_left()  #To next phase
            while(ob==0):
                ob=bot.forward()
            ob=bot.turn_right()
            path_num+=1             
            path1(path_num,ob)
        else:
            path2_block(path_num)
    else:
        ob=bot.forward() #if ob=2
        if(ob==2):
            ob=bot.turn_left()
            if(ob):
                bot.turn_left()
                ob=bot.turn_left()  #To next phase
                while(ob==0):
                    ob=bot.forward()
                ob=bot.turn_right()
                path_num+=1             
                path1(path_num,ob)
            else:
                path2_block(path_num)
        else:
            ob=bot.turn_left()
            if(ob):
                path2_double_block(path_num)
            else:
                ob=bot.forward() #if ob=2
                if(ob==2):
                    path2_double_block(path_num)
                else:
                    path1(path_num,ob)

def path2_block(path_num):
    ob=bot.forward() #if ob=2
    if(ob==2):
        bot.turn_left()
        ob=bot.turn_left()  #To next phase
        while(ob==0):
            ob=bot.forward()
        ob=bot.turn_right()
        path_num+=1             
        path1(path_num,ob)
    else:
        ob=bot.turn_right()
        if(ob):
            ob=bot.turn_left()
            if(ob):
                bot.turn_left()
                ob=bot.turn_left()  #To next phase
                while(ob==0):
                    ob=bot.forward()
                ob=bot.turn_right()
                path_num+=1             
                path1(path_num,ob)
            else:
                path2_block(path_num)
        else:
            ob=bot.forward()        #if ob=2
            if(ob==2):
                ob=bot.turn_left()
                if(ob):
                    bot.turn_left()
                    ob=bot.turn_left()  #To next phase
                    while(ob==0):
                        ob=bot.forward()
                    ob=bot.turn_right()
                    path_num+=1             
                    path1(path_num,ob)
                else:
                    path2_block(path_num)
            else:
                ob=bot.turn_left()
                if(ob):
                    path2_double_block(path_num)
                else:
                    path1(path_num,ob)

def path2_double_block(path_num):
    ob=bot.turn_left()
    if(ob):
        print("error")
    else:
        ob=bot.forward() #if ob=2
        if(ob==2):
            print("error")
        else:
            ob=bot.turn_right()
            if(ob):
                bot.turn_left()
                ob=bot.turn_left()  #To next phase
                while(ob==0):
                    ob=bot.forward()
                ob=bot.turn_right()
                path_num+=1             
                path1(path_num,ob)
            else:
                path2_block(path_num)

# def check(map,bot):#bot紀錄自己的map
#     send_instruction("C")
#     instruction_res=get_information()
#     x=bot.position[0]
#     y=bot.position[1]
#     #x+1
#     # if(map[x+1][y+1]==0):
#     #     bot.map[x+1][y+1]=2
#     # else:
#     #     bot.map[x+1][y+1]=1

#     if(map[x+1][y]==0):
#         bot.map[x+1][y]=2
#     else:
#         bot.map[x+1][y]=1
    
#     # if(map[x+1][y-1]==0):
#     #     bot.map[x+1][y-1]=2
#     # else:
#     #     bot.map[x+1][y-1]=1
#     #x
#     if(map[x][y+1]==0):
#         bot.map[x][y+1]=2
#     else:
#         bot.map[x][y+1]=1
    
#     if(map[x][y-1]==0):
#         bot.map[x][y-1]=2
#     else:
#         bot.map[x][y-1]=1
#     #x-1
#     # if(map[x-1][y+1]==0):
#     #     bot.map[x-1][y+1]=2
#     # else:
#     #     bot.map[x-1][y+1]=1
    
#     if(map[x-1][y]==0):
#         bot.map[x-1][y]=2
#     else:
#         bot.map[x-1][y]=1

#     # if(map[x-1][y-1]==0):
#     #     bot.map[x-1][y-1]=2
#     # else:
#     #     bot.map[x-1][y-1]=1
#     '''if(bystep):
#         print_map(bot.map)#若為step by step模式,則每次紀錄完map後,就印出bot的map
# 	'''

def print_map(bot):
    os.system("cls")
    
    print('\\',end=' ')
    for i in range(0,bot.y_size):
        print(i,end='  ')
    print("")
    for j in range(0,bot.y_size):
        print(j,end=' ')
        for i in range(0,bot.x_size):
            if(bot.map[i][j]==0):#還沒確認過的位置
                print(no_check_sign,end=' ')
            elif(bot.map[i][j]==1): #確認過有障礙物
                print(obstacle_sign,end='') 
            elif(bot.map[i][j]==2):#確認過無障礙物
                print(no_obstacle_sign,end=' ')
            else:
                print(bot.map[i][j],end=' ') #自己位置的狀態

		#	elif(map[i][j]==3):
		#		print("🟩",end=' ')
		#	elif(map[i][j]==4):
		#		print("🟦",end=' ')
		#	else:
		#		print(map[i][j],end=' ')
        print("")


def create_thread(func):
	# 创建线程并将其设置为守护线程
	my_thread = threading.Thread(target=func)
	my_thread.setDaemon(True)

	# 启动线程
	my_thread.start()
	return my_thread

bot=Bot()

count=0
instructions=['f','b','r','l']
path_number=0
ob=0
while(1):
    instruction=input("傳輸的指令")
    if(instruction=='script1'):
        path1(path_number,ob)
    else:
        send_instruction(instruction)
        data=get_information()
        print(data)










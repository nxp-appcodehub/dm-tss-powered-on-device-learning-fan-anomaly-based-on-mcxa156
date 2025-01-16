'''
/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
'''
import tkinter as tk
from tkinter import ttk
import serial
from serial.tools import list_ports
from tkinter import filedialog
import datetime
g_isWorking = False

def on_start():
    global g_isWorking
    global folder_path
    

    stop_button.config(state=tk.NORMAL)
    start_button.config(state=tk.DISABLED)
    static_text3.config(text="class:%s, window len:%s" % (variable.get(), wnd_len_var.get()))
    static_text4.config(text="start sampling")
    g_isWorking = True
    valid_data_count = 0
    portx = vcom_sel_var.get().split(' -')[0]
    baudrate = baudrate_var.get() 
    try:
        ser = serial.Serial(portx,baudrate)
    except Exception as e:
       static_text4.config(text="Error: " + str(e))
       ser.close()
       return
    save_num=int(wnd_len_var.get())* int(samp_cnt_var.get())+1
    with open(f"{folder_path}/{variable.get()}.txt", 'w') as f:
        pgs_bar.place(x=10, y=275)
        while g_isWorking:
            data = ser.readline().decode('utf-8').strip()  #
            if data:
                f.write(data + '\n') 
                f.flush()
                valid_data_count += 1
                pgs_bar['value'] += float(100/save_num);
                root.update()
            if valid_data_count >= save_num:
                pgs_bar['value'] = 100
                root.update()
                break
                
    with open(f"{folder_path}/{variable.get()}.txt", 'r') as f:
        lines = f.readlines()[1:]  
        float_list = [float(x) for line in lines for x in line.split()]
    new_content = ""
    for i, num in enumerate(float_list):
        new_content += str(num) + " "
        if (i + 1) % (int(wnd_len_var.get())*3) == 0:
            new_content = new_content.rstrip()  
            new_content += "\n"
    
    new_content = new_content.rstrip('\n')   
    
    
    
    if valid_data_count >= save_num:
        timestamp = datetime.datetime.now().strftime("%Y_%m%d_%H%M%S")   
        file_name = f"{folder_path}/{variable.get()}_{timestamp}.csv"  
        with open(file_name, 'w') as f:
             f.write(new_content)
             static_text4.config(text="CSV file save end")
    else:
        static_text4.config(text="Data receive interrupted")
        pgs_bar.place_forget()
               
def on_stop():
    global g_isWorking
  
    static_text4.config(text="")
    start_button.config(state=tk.NORMAL)
    stop_button.config(state=tk.DISABLED)  
    pgs_bar['value'] = 0              #        stop_button.config(state=tk.DISABLED)
    pgs_bar.place_forget()
    static_text3.config(text="")
    static_text4.config(text="")
    g_isWorking=False

def scan_serial():
    global vcom_sel_var
    global root
    global option_vcom_sel
    
    ports = list(list_ports.comports())
    
    for port in ports:
        print(port)
    ports = ['None'] + ports
    vcom_sel_var.set(ports[-0])  
    
    option_vcom_sel.destroy()
    option_vcom_sel= tk.OptionMenu(root, vcom_sel_var, *ports)
    option_vcom_sel.place(x=240, y=7)
    
def select_folder():
    global folder_path
    
    folder_path = filedialog.askdirectory(initialdir= './')
    if folder_path:
        folder_path_label.config(text="Selected Folder: " + folder_path)
        
#
def main():
    global root ,ser
   
    row = 0
    root = tk.Tk()
    root.title("eIQ Time Series Suite dataset maker")
    root.geometry("560x400")  
    root.resizable(False, False)  
    
    global vcom_sel_var
    vcom_sel_var = tk.StringVar(root)
    ports = list(list_ports.comports())
    row += 1
    for port in ports:
        print(port)
    ports = ['None'] + ports
    vcom_sel_var.set(ports[-0]) 
    global option_vcom_sel
    option_vcom_sel= tk.OptionMenu(root, vcom_sel_var, *ports)
    option_vcom_sel.place(x=240, y=7)
    
    
    
   
    global st_vcom_sel
    st_vcom_sel = tk.Label(root, text="COM Port:")
    st_vcom_sel.place(x=10, y=7,width=70)
    
    global scan_button
    scan_button = tk.Button(root, text="Scan Port", command=scan_serial)
    scan_button.place(x=140, y=7,width=80)
    
    
    global baudrate_var
    baudrate_var = tk.StringVar(root)
    baudrate_var.set("9600") 
    option_baudrate = tk.OptionMenu(root, baudrate_var, "9600", "14400", "19200", "38400", "57600", "115200","230400")
    option_baudrate.place(x=140, y=42, width=80)
    
    global static_textw
    static_textw = tk.Label(root, text="Baudrate_set:")
    static_textw.place(x=10, y=42,width=80)
    
    row -= 1
    #global scan_button
   # scan_button = tk.Button(root, text="rescan", command=change_options)
    #scan_button.place(x=10, y=5,width=60)
  
    row += 2
    
    global variable
    variable = tk.StringVar(root)
    variable.set("A")  
    global option_menu
    option_menu = tk.OptionMenu(root, variable, "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L")
    option_menu.place(x=140, y=75,width=80)
    
    global static_text
    static_text = tk.Label(root, text="Class_select:")
    static_text.place(x=10, y=75,width=80)
    row += 1
    
    global wnd_len_var
    wnd_len_var = tk.StringVar(root)
    wnd_len_var.set("128")  
    option_wnd_len = tk.OptionMenu(root, wnd_len_var, "64", "128", "256", "512", "1024", "2048")
    option_wnd_len.place(x=140, y=110,width=80)
    
    global static_text2
    static_text2 = tk.Label(root, text="Window_length:")
    static_text2.place(x=10, y=110,width=100)
    row += 1
    
    global samp_cnt_var
    samp_cnt_var = tk.StringVar(root)
    samp_cnt_var.set("30")  
    option_samp_cnt = tk.OptionMenu(root, samp_cnt_var, "15", "30", "50", "80", "120", "150", "200")
    option_samp_cnt.place(x=140, y=145,width=80)
    
    global static_text_samp_cnt
    static_text_samp_cnt = tk.Label(root, text="Sample_count:")
    static_text_samp_cnt.place(x=10, y=145,width=100)
    global folder_path
    folder_path = './'
    select_folder_button = tk.Button(root, text="Select Folder", command=select_folder)
    select_folder_button.place(x=10, y=180,width=100)
    global folder_path_label
    folder_path_label = tk.Label(root, text="Selected Folder: ")
    folder_path_label.place(x=140, y=180)
    
    row += 1
   
    global start_button
    start_button = tk.Button(root, text="Start sampling", command=on_start)
    start_button.place(x=10, y=215,width=100)
    
    global stop_button
    stop_button = tk.Button(root, text="Stop & clear", command=on_stop, state=tk.DISABLED)
    stop_button.place(x=140, y=215,width=80)
    row += 1

    global static_text3
    static_text3 = tk.Label(root, text="")
    static_text3.place(x=10, y=240,width=160)
    global static_text4
    static_text4 = tk.Label(root, text="")
    static_text4.place(x=10, y=280)
    global pgs_bar
    pgs_bar = ttk.Progressbar(root, orient="horizontal", length=200, mode="determinate")


    root.mainloop()
if __name__ == "__main__":
    main()
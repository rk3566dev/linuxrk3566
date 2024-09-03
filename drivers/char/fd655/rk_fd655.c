
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <linux/ioctl.h>
#include <linux/device.h>

#include <linux/errno.h>
#include <linux/mutex.h>


#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <linux/fcntl.h>
#include <linux/poll.h>

#include <linux/sched.h>


//#include <linux/pinctrl/pinconf-sunxi.h>
//#include <linux/sys_config.h>
#include <linux/io.h>
#include <linux/of_gpio.h>


#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/clk.h>
//#include <linux/sys_config.h>
//#include <linux/sys_config.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
//#include <linux/power/scenelock.h>

#include "rk_fd655.h"

//static struct gpio_config	key_io_clk;
//static struct gpio_config	key_io_dat;
static int usb4g_power_pin = 0;
static int usb4g_pwrkey_pin = 0;
static int sata_pwr_en = 0;
static FD655_DEV *pdata = NULL;

/** 
 * @brief   转换字符为数码管的显示码
 * @param   cTemp 待转换为显示码的字符
 * @return  显示码值,8位无符号
 * @note    码值见BCD_decode_tab[LEDMAPNUM]，如遇到无法转换的字符返回0  
 */ 
static u_int8 Led_Get_Code(char cTemp)
{
	u_int8 i, bitmap=0x00;
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	for(i=0; i<LEDMAPNUM; i++)
	{
		if(LED_decode_tab[i].character == cTemp)
		{
			bitmap = LED_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}


/*******************************************************************************
*函数名	： FD655_Start()；
*参数	   	:  无		 																	   
*功能描述 	:  FD655操作起始
*函数说明 	： 
*返回值	：  无																   
******************************************************************************/
static void FD655_Start( FD655_DEV *dev )		 	// 操作起始
{
//	FD655_SDA_D_OUT;				 /* 设置SDA为输出方向 */
//	FD655_SCL_D_OUT;				 /* 设置SCL为输出方向 */	
//	FD655_SDA_SET;  				/*发送起始条件的数据信号*/
    gpio_direction_output(dev->dat_pin, 1);
//	FD655_SCL_SET;
	gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SDA_CLR;					  /*发送起始信号*/
    gpio_direction_output(dev->dat_pin, 0);
	FD655_DELAY_1us;      
//	FD655_SCL_CLR;					  /*钳住I2C总线，准备发送或接收数据 */
    gpio_direction_output(dev->clk_pin, 0);
}					     

/*******************************************************************************
*函数名	： FD655_Stop()；
*参数	   	:  无		 																	   
*功能描述 	:  FD655操作结束
*函数说明 	： 
*返回值	：  无																   
******************************************************************************/
static void FD655_Stop( FD655_DEV *dev )				  // 操作结束
{
//	FD655_SDA_D_OUT;					 /* 设置SDA为输出方向 */	
//	FD655_SCL_D_OUT;				 	/* 设置SCL为输出方向 */	
//	FD655_SDA_CLR;
//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
    gpio_direction_output(dev->dat_pin, 0);
	FD655_DELAY_1us;
//	FD655_SCL_SET;
    gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SDA_SET;						 /*发送I2C总线结束信号*/
    gpio_direction_output(dev->dat_pin, 1);
    FD655_DELAY_1us;
}

/*******************************************************************************
*函数名	： FD655_Writebyte( uchar dat)；
*参数	   	:  dat,一个字节数据写入		 																	   
*功能描述 	:  FD655写入一个字节
*函数说明 	： 
*返回值	：  无																   
******************************************************************************/
static void FD655_Writebyte(u_int8 dat,FD655_DEV *dev)		 
{
	u_int8 i;
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	for( i = 0; i != 8; i++ )
	{
		if( dat & 0x80 ) 
		{
		   // FD655_SDA_SET;
		   gpio_direction_output(dev->dat_pin, 1);
		}
		else 
		{
		   // FD655_SDA_CLR;
		   gpio_direction_output(dev->dat_pin, 0);
		}
		FD655_DELAY_1us;
	//	FD655_SCL_SET;
	    gpio_direction_output(dev->clk_pin, 1);
		dat <<= 1;
		FD655_DELAY_1us;  // 可选延时
	//	FD655_SCL_CLR;
	    gpio_direction_output(dev->clk_pin, 0);
	}
	//FD655_SDA_SET;
	gpio_direction_output(dev->dat_pin, 1);
	FD655_DELAY_1us;
	//FD655_SCL_SET;
	gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
	//FD655_SCL_CLR;
	gpio_direction_output(dev->clk_pin, 0);
}
#if 0
/*******************************************************************************
*函数名	：  uchar FD655_Readbyte( )；
*参数	   	:  	无	 																	   
*功能描述 	:   CPU读取FD655的一个字节
*函数说明 	：  
*返回值	：  dat, 读取一个字节														
******************************************************************************/
static u_int8 FD655_Readbyte(FD655_DEV *dev)				  
{
	u_int8 dat,i;
	//FD655_SDA_D_IN;
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	dat = 0;
	for( i = 0; i != 8; i++ )
	{
		FD655_DELAY_1us;  // 可选延时
		//FD655_SCL_SET;
		gpio_direction_output(dev->clk_pin, 1);
		FD655_DELAY_1us;  // 可选延时
		dat <<= 1;
		//if( FD655_SDA_IN ) 
		if( gpio_get_value(dev->dat_pin) ) 
			dat++;
		//FD655_SCL_CLR;
		gpio_direction_output(dev->clk_pin, 0);
	}
//	FD655_SDA_D_OUT;
//	FD655_SDA_SET;
    gpio_direction_output(dev->dat_pin, 1);
	FD655_DELAY_1us;
//	FD655_SCL_SET;
    gpio_direction_output(dev->clk_pin, 1);
	FD655_DELAY_1us;
//	FD655_SCL_CLR;
    gpio_direction_output(dev->clk_pin, 0);
	return dat;
}
#endif
/*******************************************************************************
*函数名	： FD655_Command( uchar cmd)；
*参数	   	:  cmd, 控制命令，具体命令请参见FD655.H		 																	   
*功能描述 	:  对FD655进行控制，
*函数说明 	： 对其亮度，睡眠，按键显示开启等功能操作
*返回值	：  无																   
******************************************************************************/
void FD655_Command( u_int8 cmd ,FD655_DEV *dev)		  		
{								
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);	
	FD655_Start(dev);
	FD655_Writebyte(SET,dev);
	FD655_Writebyte(cmd,dev);
	FD655_Stop(dev);	
}

/*******************************************************************************
*函数名	： FD655_Disp( uchar address,uchar dat)；
*参数	   	:  address, 位地址，dat,段数据		 																	   
*功能描述 	:  对FD655单个位进行操作显示函数
*函数说明 	： add 68,6a,6c,6e 66对应DIG1-DIG4以及DIG5 dat表示显示的段码数据共阴
*返回值	：  无																   
******************************************************************************/
void FD655_Disp(u_int8 address ,u_int8 dat,FD655_DEV *dev)
{
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	FD655_Start(dev);
	FD655_Writebyte(address,dev);
	FD655_Writebyte(dat,dev);
	FD655_Stop(dev);
}
#if 0

/*******************************************************************************
*函数名	： uchar FD655_KeyScan()；
*参数	   	:  add, 位地址，dat,段数据		 																	   
*功能描述 	:  FD655按键扫描函数
*函数说明 	： 如若没按键 ，返回0 ，如果有按键，返回按键值
*返回值	：  keytemp , 返回按键代码																   
******************************************************************************/
uchar FD655_KeyScan(FD655_DEV *dev)   			
{	
	u_int8 keytemp;
	FD655_Start(dev);
	FD655_Writebyte( READKEY ,dev);
	keytemp=FD655_Readbyte(dev);
	FD655_Stop(dev);
	if((keytemp&0x40)==0)
	{	
		keytemp=0;
	}
	return keytemp;	
}

#endif

/*******************************************************************************
*函数名	： LedShow(uchar *acFPStr)；
*参数	   	:  无		 																	   
*功能描述 	:  输入4位ASCII码让数码管显示
*函数说明 	： 如输入"OPEN"	，则数码管既可显示OPEN
*返回值	：  无																   
******************************************************************************/
void LedShow( u_int8 *acFPStr, FD655_DEV *dev)
{
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	u_int8 i, iLenth;
	u_int8 dat[5]={0};
	if( strcmp(acFPStr, "") == 0 )
	{
		return;
	}
	iLenth = strlen(acFPStr);
	if(iLenth>5)
		iLenth = 5;
	
	for(i=0; i<iLenth; i++)
	{
		dat[i] = Led_Get_Code(acFPStr[i]);
	}
	//Send display data
	
	FD655_Disp(DIG1,dat[0],dev);
	FD655_Disp(DIG2,dat[1],dev);
	FD655_Disp(DIG3,dat[2],dev);
	FD655_Disp(DIG4,dat[3],dev);
	FD655_Disp(DIG5,dat[4],dev);
}


void fd655_lte_onoff(int i)
{
	if(i==1){
	//printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
//open 4G power
    if (gpio_is_valid(usb4g_power_pin)) {
    	msleep(4000);
    	printk("---%s----open 4G power pin %d\n",__func__,usb4g_power_pin);
    	gpio_direction_output(usb4g_power_pin, 1);
    	if (gpio_is_valid(usb4g_pwrkey_pin)) {
				gpio_direction_output(usb4g_pwrkey_pin, 1);
				msleep(35);
				gpio_direction_output(usb4g_pwrkey_pin, 0);
				msleep(505);
				gpio_direction_output(usb4g_pwrkey_pin, 1);
				printk("---%s----open 4G power key pin %d\n",__func__,usb4g_pwrkey_pin);
			}
		}
	/*****add end**********/
	
		printk("fd655 4G on now.............................\r\n");

	}else{
    
    if (gpio_is_valid(usb4g_power_pin)) {
    	if (gpio_is_valid(usb4g_pwrkey_pin)) {
    		gpio_direction_output(usb4g_pwrkey_pin, 0);
    	}
    	gpio_direction_output(usb4g_power_pin, 0);
    }
		printk("fd655_disp off now.............................\r\n");


	}
    return;
}
EXPORT_SYMBOL(fd655_lte_onoff);


static int fd655_dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
    file->private_data = pdata;
	FD655_Command(FD655SYSON,pdata);
	//printk("fd655_dev_open now.............................\r\n");
    return 0;
}
#ifdef CONFIG_PWM_LED
//extern bool pwm_led_flag;
extern void pwm_led_onoff(int i);
#endif

void fd655_disp_onoff(int i)
{
	printk("fd655_disp on now.............xfwu................\r\n");
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	if(i==1){
		FD655_Command(FD655SYSON,pdata);
//		gpio_direction_output(pwr_led, 1);		
		printk("fd655_disp on now.............................\r\n");
		#ifdef CONFIG_PWM_LED
		//pwm_led_flag = true;
		pwm_led_onoff(1);
		#endif
	}else{
			
		printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
			FD655_Disp(DIG1,0x00,pdata);
			FD655_Disp(DIG2,0x00,pdata);
			FD655_Disp(DIG3,0x00,pdata);
			FD655_Disp(DIG4,0x00,pdata);
			FD655_Disp(DIG5,0x00,pdata);
			FD655_Command(0x00,pdata);
		printk("fd655_disp off now.............................\r\n");
		#ifdef CONFIG_PWM_LED
		//pwm_led_flag = false;
		pwm_led_onoff(0);
				//msleep(100);
		#endif
//		gpio_direction_output(pwr_led, 0);
	}
    return;
}
EXPORT_SYMBOL(fd655_disp_onoff);
static struct class *fd65_class;

static ssize_t store_fd655_poweroff(struct class *cls,struct class_attribute *attr,
               const char *buf, size_t count)
{
   //  int ret;
               int val=0;
               char reg;
               if (kstrtoint(buf, 0, &val))
                       return -EINVAL;
                       reg = (char)val;
               //      gpio_set_value(GPIO0_C3,reg);\]
               if(reg ==1){
                  fd655_disp_onoff(1);
               }else{
                       fd655_disp_onoff(0);
               };
                      //return ret;
                        //}
               return count;
}


static struct class_attribute fd65_class_attrs[] = {
        __ATTR(fd655, 0644,  NULL, store_fd655_poweroff),
};

static void create_fd655_attrs(void) {
        int i;
        fd65_class= class_create(THIS_MODULE, "fd65ctl");
		if (IS_ERR(fd65_class)) {
			pr_err("create workledctl debug class fail\n");
			return;
        }
        for (i = 0; i < ARRAY_SIZE(fd65_class_attrs); i++) {
         if (class_create_file(fd65_class,&fd65_class_attrs[i]))
            pr_err("create workled attribute %s fail\n", fd65_class_attrs[i].attr.name);
        }
}

void fd655_disp_powerDown(void){
	
			FD655_Disp(DIG1,0x00,pdata);
			FD655_Disp(DIG2,0x00,pdata);
			FD655_Disp(DIG3,0x00,pdata);
			FD655_Disp(DIG4,0x00,pdata);
			FD655_Disp(DIG5,0x00,pdata);
			FD655_Command(0x00,pdata);
			FD655_Command(~FD655SYSON,pdata);
			gpio_direction_output(pdata->dat_pin, 0);
			gpio_direction_output(pdata->clk_pin, 0);
	
}
EXPORT_SYMBOL(fd655_disp_powerDown);


static int fd655_dev_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	//FD655_Command(SLEEP,pdata);
	
	LedShow(" ",pdata);
	//modify for display longer
/*	
	FD655_Command(~FD655SYSON,pdata);
	printk("succes to close  fd655_dev.............\n");
*/
	//printk("try not to close  fd655_dev.............\n");
    return 0;
}



static ssize_t  fd655_dev_write(struct file *filp, const char __user *buf,		size_t count, loff_t *f_pos)
{

		FD655_DEV *dev;
		unsigned long	missing;
		size_t			status = 0, i = 0;
		char data[5] = {0};
		char tmp[5]= {0};
		dev = filp->private_data;
		if (count >5)		
			count = 5;
		missing = copy_from_user(data, buf, count);
		//printk("write buf: %s \r\n",data);
//		printk(KERN_ERR "---xfwu-------%s---%d----write buf: %s\r\n",__func__,__LINE__,data);
		if(pdata->test_flag==1)
		{
			FD655_Disp(DIG1,0x07,dev);
			FD655_Disp(DIG2,0x66,dev);
			FD655_Disp(DIG3,0x6a,dev);
			FD655_Disp(DIG4,0x46,dev);
			FD655_Disp(DIG5,0x00,dev);
			status = count;
		}
		else
		{		
			if (missing == 0)
			{	
		     	for(i=0; i<count; i++)
				{
			//		tmp[i] = data[i];
			    tmp[i] = Led_Get_Code(data[i]);
				//	printk("Led_Get_Code buf: %x \r\n",tmp[i]);
				}
				
				FD655_Disp(DIG1,tmp[0],dev);
				FD655_Disp(DIG2,tmp[1],dev);
				FD655_Disp(DIG3,tmp[2],dev);
				FD655_Disp(DIG4,tmp[3],dev);
				FD655_Disp(DIG5,data[4],dev);
				status = count;
			} 
		}
		//printk("fd655_dev_write count : %d\n",count );
		return status;

}



static struct file_operations fd655_fops = {
	.owner		=	THIS_MODULE,
	.open		=	fd655_dev_open,
	.release	=	fd655_dev_release,
	.write      =   fd655_dev_write,
	
};
static struct miscdevice fd655_device = {
	.minor	=	MISC_DYNAMIC_MINOR,
	.name	=	DEV_NAME,
	.fops	=	&fd655_fops,
};




static int register_fd655_driver(void)
{
    int ret = 0;
    ret = misc_register(&fd655_device);
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	if(ret)
		printk("%s: failed to add fd655 module\n", __func__);
	else
		printk("%s: Successed to add fd655  module \n", __func__);
    return ret;
}

static void deregister_fd655_driver(void) 
{
	//int ret = 0;
	//ret = misc_deregister(&fd655_device);
	misc_deregister(&fd655_device);
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	//if(ret)
	//	printk("%s: failed to deregister fd655 module\n", __func__);
	//else
	//	printk("%s: Successed to deregister fd655  module \n", __func__);
}

static ssize_t vplay_flag_show(struct class *dev,
                 struct class_attribute *attr, char *buf){
	return sprintf(buf, "%d\n", pdata->vplay_flag);	
}


static ssize_t vplay_flag_store(struct class *dev,
                 struct class_attribute *attr,
                 const char *buf, size_t count){
    sscanf(buf, "%d", &pdata->vplay_flag);
	return count;
}
static CLASS_ATTR_RW(vplay_flag);

static ssize_t vpause_flag_show(struct class *dev,
                 struct class_attribute *attr, char *buf){
        return sprintf(buf, "%d\n", pdata->vpause_flag);
}

static ssize_t vpause_flag_store(struct class *dev,
                 struct class_attribute *attr,
                 const char *buf, size_t count){
    sscanf(buf, "%d", &pdata->vpause_flag);
        return count;
}
static CLASS_ATTR_RW(vpause_flag);



static ssize_t test_flag_show(struct class *dev,
                 struct class_attribute *attr, char *buf){
        return sprintf(buf, "%d\n", pdata->test_flag);
}

static ssize_t test_flag_store(struct class *dev,
                 struct class_attribute *attr,
                 const char *buf, size_t count){
    sscanf(buf, "%d", &pdata->test_flag);
        return count;
}
static CLASS_ATTR_RW(test_flag);

static ssize_t panel_show(struct class *dev,
                 struct class_attribute *attr, char *buf){
	return sprintf(buf, "%s\n", pdata->wbuf);
}

static ssize_t panel_store(struct class *dev,
                 struct class_attribute *attr,
                 const char *buf, size_t count){

        FD655_Command(FD655SYSON,pdata);
	sscanf(buf, "%s", pdata->wbuf);
	LedShow(pdata->wbuf,pdata);
	return count;
}
static CLASS_ATTR_RW(panel);

static struct attribute *fd655_class_attrs[] = {
	&class_attr_vplay_flag.attr,
	&class_attr_vpause_flag.attr,
	&class_attr_test_flag.attr,
	&class_attr_panel.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fd655_class);

static struct class fd655_class = {
	.name =		"fd655",
	.owner =	THIS_MODULE,
	.class_groups = fd655_class_groups,
};


static int fd655_driver_remove(struct platform_device *pdev)
{
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
  fd655_disp_onoff(0);
  fd655_lte_onoff(0);
    deregister_fd655_driver();
	class_unregister(&pdata->sysfs);
#ifdef CONFIG_OF
	gpio_free(pdata->clk_pin);
   	gpio_free(pdata->dat_pin);	
	kfree(pdata);
#endif
    return 0;
}

static int fd655_driver_suspend(struct platform_device *dev, pm_message_t state)
{
    //gpio_free(pdata->clk_pin);
   	//gpio_free(pdata->dat_pin);
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
   	fd655_disp_onoff(0);
   	printk("fd655_driver_suspend \n");
	return 0;
}

static int fd655_driver_resume(struct platform_device *dev)
{
	//int ret = -1;
    //printk("fd655_driver_resume");
	//ret = gpio_request(pdata->clk_pin, DEV_NAME);
	//if(ret){
	//	printk("---%s----can not request pin %d\n",__func__,pdata->clk_pin);
	//}	
	//ret = gpio_request(pdata->dat_pin, DEV_NAME);
	//if(ret){
	//	printk("---%s----can not request pin %d\n",__func__,pdata->dat_pin);
	//}	
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
    fd655_disp_onoff(1);
    printk("fd655_driver_resume \n");
    return 0;
}



static int fd655_driver_probe(struct platform_device *pdev)
{
	int state=-EINVAL;

	char buf[32];
	int ret;
	const char *str;
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	unsigned int desc;
	//unsigned int hdmi_hdp;

	printk("==%s==\n", __func__);

    if (!pdev->dev.of_node) {
				printk("fd655_driver: pdev->dev.of_node == NULL!\n");
				state = -EINVAL;
				goto get_fd655_node_fail;
	}

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        printk("platform data is required!\n");
        state = -EINVAL;
        goto get_fd655_mem_fail;
    }
	fd65_class = kzalloc(sizeof(struct class), GFP_KERNEL);
       if (fd65_class == NULL)
	          return -ENOMEM;

	snprintf(buf, sizeof(buf), "clk_pin");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		pdata->clk_pin = desc;  //121 is my board IO,prevent other board use 
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}
	snprintf(buf, sizeof(buf), "dat_pin");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		pdata->dat_pin = desc;  //122 is my board IO,prevent other board use
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}
	
	
		snprintf(buf, sizeof(buf), "usb4g_power_pin");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		usb4g_power_pin = desc;  //122 is my board IO,prevent other board use
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}
	
			snprintf(buf, sizeof(buf), "usb4g_pwrkey_pin");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		usb4g_pwrkey_pin = desc;  //122 is my board IO,prevent other board use
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}


//sata_pwr_en
		snprintf(buf, sizeof(buf), "sata_pwr_en");
	ret = of_property_read_string(pdev->dev.of_node, buf, &str);
	if(!ret){
		desc = of_get_named_gpio_flags(pdev->dev.of_node, buf, 0, NULL);		
		sata_pwr_en = desc;  //122 is my board IO,prevent other board use
		printk("%s: %d\n", buf, desc);
	}else{
		printk("cannot find resource \"%s\"\n", buf);
	}


    ret = gpio_request(pdata->clk_pin, DEV_NAME);
	if(ret){
		printk("---%s----can not request pin %d\n",__func__,pdata->clk_pin);
		goto get_fd655_mem_fail ;
	}	
	ret = gpio_request(pdata->dat_pin, DEV_NAME);
	if(ret){
		printk("---%s----can not request pin %d\n",__func__,pdata->dat_pin);
		goto get_fd655_mem_fail ;
	}
	pdata->vpause_flag = 0;
  pdata->vplay_flag = 0;
	pdata->test_flag = 0;

	pdata->sysfs = fd655_class;

	ret= class_register(&pdata->sysfs);
	if(ret < 0)
		goto error_sysfs; 
	
	platform_set_drvdata(pdev, pdata);
	
    register_fd655_driver();
	
	/******add by louis for disply when machine start************/
	FD655_Command(FD655SYSON,pdata);
			FD655_Disp(DIG1,0x7f,pdata);
			FD655_Disp(DIG2,0x7f,pdata);
			FD655_Disp(DIG3,0x7f,pdata);
			FD655_Disp(DIG4,0x7f,pdata);
			FD655_Disp(DIG5,0x7f,pdata);
			

	/*****add end**********/



	/******add by xiangqian for 4G************/
//open 4G power
   fd655_lte_onoff(1);
	/*****add end**********/
	
	/******add by xiangqian for sata************/
//open sata power
    if (gpio_is_valid(sata_pwr_en)) {
    	msleep(1000);
    	printk("---%s----open sata power pin %d\n",__func__,usb4g_power_pin);
    	gpio_direction_output(sata_pwr_en, 1);
		}
	/*****add end**********/
 create_fd655_attrs();
    return 0;
	
    get_fd655_mem_fail:
			kfree(pdata);
    get_fd655_node_fail:
    error_sysfs:	
    return state;
}



static const struct of_device_id fd655_dt_match[]={
	{	.compatible = "rockchip,fd655_dev", .data = NULL},
	{ /* sentinel */ }
};


static struct platform_driver fd655_driver = {
    .probe      = fd655_driver_probe,
    .remove     = fd655_driver_remove,
    .suspend    = fd655_driver_suspend,
    .resume     = fd655_driver_resume,
    .driver     = {
        .name   = "fd655_dev",
		.owner	= THIS_MODULE,
        .of_match_table = of_match_ptr(fd655_dt_match),
    },
};

static int __init fd655_driver_init(void)
{
    printk( "Fd655 Driver init.\n");
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
	if (platform_driver_register(&fd655_driver)) {
		printk("gpio user platform_driver_register  fail\n");
		return -1;
	}
	//printk("==%s==\n", __func__);
	return 0;
}

static void __exit fd655_driver_exit(void)
{
    printk("Fd655 Driver exit.\n");
	printk(KERN_ERR "---xfwu-------%s---%d-\n",__func__,__LINE__);
    platform_driver_unregister(&fd655_driver);
}

module_init(fd655_driver_init);
module_exit(fd655_driver_exit);

MODULE_AUTHOR("kenneth");
MODULE_DESCRIPTION("fd655 Driver");
MODULE_LICENSE("GPL");

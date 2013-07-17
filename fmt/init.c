7850 // init: The initial user-level program
7851 
7852 #include "types.h"
7853 #include "stat.h"
7854 #include "user.h"
7855 #include "fcntl.h"
7856 
7857 char *argv[] = { "sh", 0 };
7858 
7859 int
7860 main(void)
7861 {
7862   int pid, wpid;
7863 
7864   if(open("console", O_RDWR) < 0){
7865     mknod("console", 1, 1);
7866     open("console", O_RDWR);
7867   }
7868   dup(0);  // stdout
7869   dup(0);  // stderr
7870 
7871   for(;;){
7872     printf(1, "init: starting sh\n");
7873     pid = fork();
7874     if(pid < 0){
7875       printf(1, "init: fork failed\n");
7876       exit();
7877     }
7878     if(pid == 0){
7879       exec("sh", argv);
7880       printf(1, "init: exec sh failed\n");
7881       exit();
7882     }
7883     while((wpid=wait()) >= 0 && wpid != pid)
7884       printf(1, "zombie!\n");
7885   }
7886 }
7887 
7888 
7889 
7890 
7891 
7892 
7893 
7894 
7895 
7896 
7897 
7898 
7899 

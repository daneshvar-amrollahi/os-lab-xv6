#include "types.h"
#include "stat.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]){
	if(argc < 2){
		printf(2, "You must enter exactly 1 number!\n");
		exit();
	}
    else
    {
		int number = atoi(argv[1]);
		printf(1, "User: calling set_sleep for %d seconds...\n" , number);


        struct rtcdate st, en;
        set_date(&st);
        printf(1, "Current system time: %d\n", st.second);
        set_sleep(number);      
        set_date(&en);	
        printf(1, "Current system time: %d\n", en.second);
        
        printf(1, "Difference: %d\n", en.second - st.second);

    }
    exit();
} 
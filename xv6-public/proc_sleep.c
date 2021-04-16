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


        struct rtcdate r;
        set_date(&r);

        //printf(1, "%d", r.second);
        
        //clock_t start, end;
        //start = clock();
        set_sleep(number);  
    
        set_date(&r);	
        //end = clock();

        //printf(1, "%d", r.second);

        //double time_taken = (double)(end - start) / (double)(CLOCKS_PER_SEC);    
        //printf(1, "Time passed is %f", time_taken);
        //cout << "Time taken for matrix construction is " << fixed << setprecision(6) << time_taken << " sec" << endl;
    }

} 
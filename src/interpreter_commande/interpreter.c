#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

extern char *optarg;
extern int optind;

int main(int argc, char* argv[]){
    double sum = 0;
    int c;
    int precision = 0;
    double param = 0.0;
    while((c=getopt(argc, argv, "a:s:m:d:ID")) != -1){
        switch(c){
            case('a'):
                param = strtod(optarg, NULL);
                if(param)
                    sum += strtod(optarg, NULL);
                else{
                    fprintf(stderr, "Wrong parameter");
                    exit(EXIT_FAILURE);
                }
                break;
            case('s'):
                param = strtod(optarg, NULL);
                if(param)
                    sum -= strtod(optarg, NULL);
                else{
                    fprintf(stderr, "Wrong parameter");
                    exit(EXIT_FAILURE);
                }
                break;
            case('m'):
                param = strtod(optarg, NULL);
                if(param)
                    sum *= strtod(optarg, NULL);
                else{
                    fprintf(stderr, "Wrong parameter");
                    exit(EXIT_FAILURE);
                }
                break;
            case('d'):
                param = strtod(optarg, NULL);
                if(param)
                    sum /= strtod(optarg, NULL);
                else{
                    fprintf(stderr, "Wrong parameter");
                    exit(EXIT_FAILURE);
                }
                break;
            case('I'):
                sum++;
                break;
            case('D'):
                sum--;
                break;
            case('?'):
                fprintf(stderr, "Unknown operation\n");
                exit(EXIT_FAILURE);
                break;
                
        }
    }

    if(atoi(argv[argc-1]) > 0){
        precision = atoi(argv[argc-1]);
    }
    else if(argc == 1 || optind == argc){
    }
    else{
        fprintf(stderr, "Error, unknown operation\n");
        return 1;
    }


    fprintf(stdout, "%.*f\n", precision, sum);

    return 0;
}
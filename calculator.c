#include<stdio.h>
#include<string.h>
#include<ctype.h>
#define max 31

void removeSpaces(char* exp){
    int i=0, j=0;
    while(exp[i]){
        if(!isspace((unsigned char)exp[i])){
            exp[j++]=exp[i];
        }
        i++;
    }
    exp[j]='\0';
}

int isOperator(char c){
    return c=='+' || c=='-' || c=='*' || c=='/';
}

int checkValidation(char* exp){
    int expsize = strlen(exp);
    if(expsize==0) return 0;

    if(!isdigit(exp[0]) || !isdigit(exp[expsize-1])) return 0;

    for(int i=0; i<expsize; i++){
        if(!isdigit(exp[i]) && !isOperator(exp[i])){
            return 0;
        }
        if(isOperator(exp[i]) && (i+1<expsize) && isOperator(exp[i+1])){
            return 0;
        }
    }
    return 1;
}

int operations(int num1, int num2, char opr, int* zeroDivError){
    switch (opr)
    {
    case '+':
        return num1+num2;
    case '-':
        return num1-num2;
    case '*':
        return num1*num2;
    case '/':
        if(num2==0){
            *zeroDivError=1;
            return 0;
        }
        return num1/num2;
    }
    return 0;
}

int main()
{
    char exp[max];
    printf("Rules:\n\t1)Expression should not exceed the length of 30 characters. \n\t2)Expression should only include digits 0-9, and operators +, -, *, /\n\n");

    printf("Enter Mathematical Expression : ");
    // scanf("%s", &exp);
    fgets(exp, sizeof(exp), stdin);
    exp[strcspn(exp, "\n")]='\0';
    
    removeSpaces(exp);

    if(!checkValidation(exp)){
        printf("Error: Invalid Expression\n");
        return 1;
    }

    int numbersArray[25];
    char operatorsArray[25];
    int numCount=0, oprCount=0, expsize=strlen(exp), i=0;

    while(i<expsize){
        int num=0;
        while(i<expsize && isdigit(exp[i])){
            num=num*10 + (exp[i]-'0');
            i++;
        }
        numbersArray[numCount++]=num;

        if(i<expsize && isOperator(exp[i])){
            operatorsArray[oprCount++]=exp[i];
            i++;
        }
    }


    int zeroDivError=0;
    for(i=0; i<oprCount; i++){
        if(operatorsArray[i]=='*' || operatorsArray[i]=='/'){
            int result = operations(numbersArray[i], numbersArray[i+1], operatorsArray[i], &zeroDivError);

            numbersArray[i]=result;

            for(int j=i+1; j<numCount-1; j++){
                numbersArray[j]=numbersArray[j+1];
            }
            for(int j=i; j<oprCount-1; j++){
                operatorsArray[j]=operatorsArray[j+1];
            }

            numCount--;
            oprCount--;
            i--;
        }
    }
    int result = numbersArray[0];
    for(i=0; i<oprCount; i++){
        result=operations(result, numbersArray[i+1], operatorsArray[i], &zeroDivError);
    }

    if(zeroDivError){
        printf("Error: Division by Zero\n");
    }
    else{
        printf("%d", result);
    }

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LENGTH 31

void removeSpaces(char *exp){
    int i=0,j=0;
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

int isValidExpression(char *exp){
    int len=strlen(exp);
    if(len==0) return 0;

    if(!isdigit(exp[0]) || !isdigit(exp[len-1])){
        return 0;
    }

    for(int i = 0 ; i < len ; i++){
        if( !isdigit(exp[i]) && !isOperator(exp[i]) ){
            return 0;
        }
        if( isOperator(exp[i]) && (i+1<len) && isOperator(exp[i+1]) ){
            return 0;
        }
    }
    return 1;
}

int evaluate(int a,int b,char op,int *divErr){
    switch(op){
        case '+': return a+b;
        case '-': return a-b;
        case '*': return a*b;
        case '/':
            if(b==0){
                *divErr = 1;
                return 0;
            }
            return a/b;
    }
    return 0;
}

int main(){
    char expression[MAX_LENGTH];

    printf("Rules:\n");
    printf(" 1) Max length: 30 characters\n");
    printf(" 2) Only digits and operators + - * /\n\n");

    printf("Enter Expression: ");
    fgets(expression, sizeof(expression), stdin);
    expression[strcspn(expression, "\n")] = '\0';

    removeSpaces(expression);

    if( !isValidExpression(expression) ){
        printf("Error: Invalid Expression\n");
        return 1;
    }

    int nums[25];
    char ops[25];
    int nIndex = 0, oIndex = 0;
    int i = 0, len = strlen(expression);

    while(i<len){
        int val = 0;
        while(i<len && isdigit(expression[i])){
            val = val*10 + (expression[i]-'0');
            i++;
        }
        nums[nIndex++] = val;

        if(i<len && isOperator(expression[i])){
            ops[oIndex++] = expression[i];
            i++;
        }
    }

    int divErr = 0;

    for(i = 0 ; i < oIndex ; i++){
        if(ops[i]=='*' || ops[i]=='/'){
            int res = evaluate(nums[i], nums[i+1], ops[i], &divErr);
            nums[i] = res;

            for(int j=i+1;j<nIndex-1;j++){
                nums[j] = nums[j+1];
            }
            for(int j=i;j<oIndex-1;j++){
                ops[j] = ops[j+1];
            }

            nIndex--;
            oIndex--;
            i--;
        }
    }

    int final = nums[0];
    for(i = 0; i < oIndex; i++){
        final = evaluate(final, nums[i+1], ops[i], &divErr);
    }

    if(divErr){
        printf("Error: Division by Zero\n");
    } else{
        printf("%d\n", final);
    }

    return 0;
}

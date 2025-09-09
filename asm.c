#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<ctype.h>

// structure for stroring one line of instructions
struct code
{
	int pc;
	int line_num;
	int opcode;
	char pnemonic[50];
	char operand[50];
	char label[50];
	struct code* next;
};

typedef struct opcode
{
	char name[50];
	int code;
}Opcode;

// pnemonics table 
Opcode opcodes[21] = 
{
	{"ldc", 0},
	{"adc", 1},
	{"ldl", 2},
	{"stl", 3},
	{"ldnl", 4},
	{"stnl", 5},
	{"add", 6},
	{"sub", 7},
	{"shl", 8},
	{"shr", 9},
	{"adj", 10},
	{"a2sp", 11},
	{"sp2a", 12},
	{"call", 13},
	{"return", 14},
	{"brz", 15},
	{"brlz", 16},
	{"br", 17},
	{"HALT", 18},
	{"data", 19},
	{"SET", 20}
};

// error handling
struct error
{
	int line_num;
	char state[100];
	struct error *next;
};

struct error *error_head_g = NULL;
int error_count_g = 0; 
int label_count_g = 0;

// storing the encountered errors as a linked list of error data structure
void append_error(int line_num_s, char *state_s)
{
	if(error_head_g == NULL)
	{
		error_head_g = (struct error*)(malloc(sizeof(struct error)));
		error_head_g->line_num = line_num_s;
		strcpy(error_head_g->state, state_s);
		error_head_g->next = NULL;
		error_count_g++;
	}
	else
	{
		struct error* temp = (struct error*)(malloc(sizeof(struct error)));
		struct error* last = error_head_g;
		strcpy(temp->state, state_s);
		temp->line_num = line_num_s;
		temp->next = NULL;
		while(last->next!= NULL)
		{
			last = last->next;
		}
		last->next = temp;
		error_count_g++;
	}
	return;
}

// inserting valid liens of code
void append_code(struct code* head, char *pnemonic_s, char *operands_s, char *label_s, int pcctr_s, int line_num_s)
{
	struct code* temp = (struct code*)(malloc(sizeof(struct code)));
	struct code* last = head;
	strcpy(temp->pnemonic, pnemonic_s);
	strcpy(temp->operand, operands_s);
	strcpy(temp->label, label_s);
	int j, flag;
	flag = 0;
	for(j=0;j<21;j++)
	{
		if (strcmp(pnemonic_s, opcodes[j].name) == 0)
		{
			flag = 1;
			temp->opcode = opcodes[j].code;
		}
	}
	if (flag == 0)
	{
		append_error(line_num_s+1, "Error : Pnemonic does not exist");
		temp->opcode = -1;
	}
	temp->pc = pcctr_s;
	temp->line_num = line_num_s;
	temp->next = NULL;
	while(last->next!= NULL)
	{
		last = last->next;
	}
	last->next = temp;
	return;
}


// combine utility functions for first level code representation
struct code* createCode(char **pnemonics, char **operands, char **labels, int ctr)
{
	struct code* head = NULL;
	int i, start=0, pcctr=0;
	for(i=0; i<ctr; i++)
	{
		if (strcmp(pnemonics[i], "")!= 0)
		{
			head = (struct code*)(malloc(sizeof(struct code)));
			strcpy(head->pnemonic, pnemonics[i]);
			strcpy(head->operand, operands[i]);
			strcpy(head->label, labels[i]);
			int j, flag;
			flag = 0;
			for(j=0;j<21;j++)
			{
				if (strcmp(pnemonics[i], opcodes[j].name) == 0)
				{
					flag = 1;
					head->opcode = opcodes[j].code;
				}
			}
			if (flag == 0)
			{
				append_error(i+1, "Error : Pnemonic does not exist");
				head->opcode = -1;
			}
			head->pc = 0;
			head->line_num = i;
			head->next = NULL;
			start = i;
			pcctr++;
			break;
		} 
	}
	if (head != NULL)
	{
		for(i=start+1;i<ctr;i++)
		{
			if(strcmp(pnemonics[i], "")!= 0)
			{
				append_code(head, pnemonics[i], operands[i], labels[i], pcctr, i);
				pcctr++;
			}
		}
		return head;
	}
	else
	{
		printf("head not initlialised");
		return NULL;
	}
}

/*
 * debug utility to check code linked list
void displayCodeLL(struct code* head)
{
	struct code* last = head;
	printf("\nCode Compact Representation\n");
	printf("PC, Label, instruction, pnemonic, operand\n");
	while(1)
	{
		printf("%d, ", last->pc);
		printf("%s, ", last->label);
		printf("%s, ", last->pnemonic);
		printf("%d, ", last->opcode);
		printf("%s\n", last->operand);
		last = last->next;
		if(last == NULL)
			break;
	}
}
*/ 


// checking for labels(names given to blocks or variables)
// returns offset on success, adds to error log if not found
int label_offset(char* label_s, struct code* head, int line_num_s, int pc_s, int type)
{
	struct code* last = head;
	int flag, offset;
	flag = 0;
	while(1)
	{
		if (strcmp(label_s, last->label) == 0)
		{
			flag = 1;
			if (type == 1)
				offset = last->pc - pc_s;
			else if(type == 0)
			{
				if (last->opcode !=20)
					offset = last->pc;
				else if(last->opcode == 20)
				{
					char *end;
					int ret;
					ret = strtol(last->operand, &end, 0);
					offset = ret;
				}
			}
			break;
		}
		last = last->next;
		if(last == NULL)
			break;
	}
	if(flag == 0)
	{
		append_error(line_num_s, "Error : Label does not exist");
	}
	return offset;
}

// checks operand to see if it a label(variable name) or a number 
long processOperand(char* operand_s, struct code* head, int line_num_s, int pc, int type)
{
	char *end;
	long ret;
	ret = strtol(operand_s, &end, 0);
	if (strcmp(end, "") != 0)
	{
		if (strcmp(operand_s, end) == 0)
		{
			/*printf("Above is a label\n");*/
			ret = label_offset(operand_s, head, line_num_s, pc, type);
		}
		else
		{
			append_error(line_num_s, "Error : Operand is not a number");
		}
	}
	return ret;	
}

// checks if there is an operand on the given line
int CheckOperandExistence(char* operand_s)
{
	int flag = 1;
	if (strcmp(operand_s, "") == 0)
	{
		flag = 0;
	}
	return flag;	
}

void dectohex(long decimalnum, char *res)
{
	int neg = 0;
	long neg_const = 16777216;
	if (decimalnum == 0)
	{
		res[0] = '0';
		res[1] = '\0';
		return;
	}
	else if(decimalnum < 0)
	{
		decimalnum = neg_const + decimalnum;
		neg = 1;
	}
    long quotient, remainder;
    int i, j;
    j = 0;
    char hexadecimalnum[100];
    quotient = decimalnum;
    while (quotient != 0)
    {
        remainder = quotient % 16;
        if (remainder < 10)
            hexadecimalnum[j++] = 48 + remainder;
        else
            hexadecimalnum[j++] = 55 + remainder;
        quotient = quotient / 16;

    }
    int k = 0;
    for (i = j-1; i >= 0; i--)
    {
        res[k] = hexadecimalnum[i];
        k++;
    }
    res[k] = '\0';
}

void printLabelList(FILE *flst, char *label, char *res_pc)
{
	int pc_pad, k;
	pc_pad = 8 - strlen(res_pc);
	for(k=0;k<pc_pad;k++)
	{
		fprintf(flst, "0");
	}	
	fprintf(flst, "%s\t", res_pc);
	for(k=0;k<8;k++)
	{
		fprintf(flst, " ");
	}
	fprintf(flst, "\t");
	fprintf(flst, "%s", label);
	fprintf(flst, ":\n");
}

// prints into binary object file
void printObjBin(FILE *fbin, char *pc, char *ins)
{
    fwrite(ins,sizeof(ins),1,fbin);
}

// print machine code into list file 
void printList(FILE *fbin, FILE *flst, char *res_pc, char *res_oc, char *res_op, char *pnemonic, char *operand)
{
	int pc_pad, op_pad, k, l=0;
	char pc[8];
	char ins[8];
	pc_pad = 8 - strlen(res_pc);
	for(k=0;k<pc_pad;k++)
	{
		fprintf(flst, "0");
		pc[l++] = '0';
	}
	fprintf(flst, "%s\t", res_pc);
	for(k=0;k<strlen(res_pc);k++)
	{
		pc[l++] = res_pc[k];
	}
	l = 0;
	op_pad = 6 - strlen(res_op);
	for(k=0;k<op_pad;k++)
	{
		fprintf(flst, "0");
		ins[l++] = '0';
	}
	fprintf(flst, "%s", res_op);
	for(k=0;k<strlen(res_op);k++)
	{
		ins[l++] = res_op[k];
	}
	op_pad = 2 - strlen(res_oc);
	for(k=0;k<op_pad;k++)
	{
		fprintf(flst, "0");
		ins[l++] = '0';
	}
	for(k=0;k<strlen(res_oc);k++)
	{
		ins[l++] = res_oc[k];
	}
	fprintf(flst, "%s\t", res_oc);
	fprintf(flst, "%s ", pnemonic);
	fprintf(flst, "%s\n", operand);
	printObjBin(fbin, pc, ins);
}

// master function for second pass
void secondPass(struct code* head, char *file_name)
{
	char name_file[50];
	strcpy(name_file, file_name);
	strcat(file_name, "_listfile.lst");
	FILE *fptr = fopen("listfile.txt", "w");
	if(fptr == NULL)
	{
	      printf("Error : Cannot open list file");   
	      exit(1);             
	}
	FILE *flst = fopen(file_name, "w");
	if(flst == NULL)
	{
	      printf("Error : cannot open list file ");   
	      exit(1);             
	}

	strcat(name_file, "_obj.o");
	FILE *fbin = fopen(name_file, "wb");
	struct code* last = head;
	int ctr = 0;
	char res_pc[32];
	char res_oc[32];
	char res_op[32];
	int pc_pad, op_pad;
	int type;
	while(1)
	{
		char lst_write[200];
		int opcode = last->opcode;
		if (opcode == 0 || opcode == 1 || opcode == 10)
		{	
			int flag = CheckOperandExistence(last->operand);
			if (flag == 0)
			{
				append_error(last->line_num+1, "Error : Operand does not exist");
			}
			else
			{
				int k;
				type = 0;
				long decoperand = processOperand(last->operand, head, last->line_num+1, last->pc, type);
				fprintf(fptr, "PC\t");
				dectohex(last->pc, res_pc);
				pc_pad = 8 - strlen(res_pc);
				for(k=0;k<pc_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_pc);
				fprintf(fptr, "Opcode\t");
				dectohex(opcode, res_oc);
				op_pad = 2 - strlen(res_oc);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_oc);
				fprintf(fptr, "Operand\t");
				dectohex(decoperand, res_op);
				op_pad = 6 - strlen(res_op);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\n", res_op);
				if (strcmp(last->label, "")!=0)
				{
					printLabelList(flst, last->label, res_pc);
				}
				printList(fbin, flst, res_pc, res_oc, res_op, last->pnemonic, last->operand);
			}
		}
		else if(opcode == 2 || opcode == 3 || opcode == 4 || opcode == 5)
		{
			int flag = CheckOperandExistence(last->operand);
			if (flag == 0)
			{
				append_error(last->line_num+1, "Error : Operand does not exist");
			}
			else
			{
				int k;
				type = 1;
				long decoperand = processOperand(last->operand, head, last->line_num+1, last->pc, type);
				decoperand = decoperand - 1;
				fprintf(fptr, "PC\t");
				dectohex(last->pc, res_pc);
				pc_pad = 8 - strlen(res_pc);
				for(k=0;k<pc_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_pc);
				fprintf(fptr, "Opcode\t");
				dectohex(opcode, res_oc);
				op_pad = 2 - strlen(res_oc);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_oc);
				fprintf(fptr, "Operand\t");
				dectohex(decoperand, res_op);
				op_pad = 6 - strlen(res_op);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\n", res_op);
				if (strcmp(last->label, "")!=0)
				{
					printLabelList(flst, last->label, res_pc);
				}
				printList(fbin, flst, res_pc, res_oc, res_op, last->pnemonic, last->operand);
			}			
		}
		else if(opcode == 13 || opcode == 15 || opcode == 16 || opcode == 17)
		{
			int flag = CheckOperandExistence(last->operand);
			if (flag == 0)
			{
				append_error(last->line_num+1, "Error : Operand does not exist");
			}
			else
			{
				int k;
				type = 1;
				long decoperand = processOperand(last->operand, head, last->line_num+1, last->pc, type);
				decoperand = decoperand - 1;
				fprintf(fptr, "PC\t");
				dectohex(last->pc, res_pc);
				pc_pad = 8 - strlen(res_pc);
				for(k=0;k<pc_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_pc);
				fprintf(fptr, "Opcode\t");
				dectohex(opcode, res_oc);
				op_pad = 2 - strlen(res_oc);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_oc);
				fprintf(fptr, "Operand\t");
				dectohex(decoperand, res_op);
				op_pad = 6 - strlen(res_op);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\n", res_op);
				if (strcmp(last->label, "")!=0)
				{
					printLabelList(flst, last->label, res_pc);
				}
				printList(fbin, flst, res_pc, res_oc, res_op, last->pnemonic, last->operand);
			}			
		}
		else if(opcode == 19 || opcode == 20)
		{
			int flag = CheckOperandExistence(last->operand);
			if (flag == 0)
			{
				append_error(last->line_num+1, "Error : Operand does not exist");
			}
			else
			{
				int k;
				type = 0;
				long decoperand = processOperand(last->operand, head, last->line_num+1, last->pc, type);
				fprintf(fptr, "PC\t");
				dectohex(last->pc, res_pc);
				pc_pad = 8 - strlen(res_pc);
				for(k=0;k<pc_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_pc);
				fprintf(fptr, "Opcode\t");
				dectohex(opcode, res_oc);
				op_pad = 2 - strlen(res_oc);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\t", res_oc);
				fprintf(fptr, "Operand\t");
				dectohex(decoperand, res_op);
				op_pad = 6 - strlen(res_op);
				for(k=0;k<op_pad;k++)
				{
					fprintf(fptr, "0");
				}
				fprintf(fptr, "%s\n", res_op);
				if (strcmp(last->label, "")!=0)
				{
					printLabelList(flst, last->label, res_pc);
				}
				printList(fbin, flst, res_pc, res_oc, res_op, last->pnemonic, last->operand);
			}
		}
		else if(opcode == 6 || opcode == 7 || opcode == 8 || opcode == 9 || opcode == 11 || opcode == 12 || opcode == 14 || opcode == 18)
		{
			int flag = CheckOperandExistence(last->operand);
			if (flag == 1)
			{
				append_error(last->line_num+1, "Error : Operand does not exist");
			}
			else
			{
				dectohex(last->pc, res_pc);
				dectohex(opcode, res_oc);
				dectohex(0, res_op);
				if (strcmp(last->label, "")!=0)
				{
					printLabelList(flst, last->label, res_pc);
				}
				printList(fbin, flst, res_pc, res_oc, res_op, last->pnemonic, last->operand);
			}			
		}
		last = last->next;
		if(last == NULL)
			break;
		ctr++;
	}
	fclose(fptr);
}

// to display errors in source
void displayErrorLL(FILE *flog)
{
	if (error_head_g == NULL)
	{
		return;
	}
	struct error* last = error_head_g;
	fprintf(flog, "\nData in the Error LL\n");
	while(1)
	{
		fprintf(flog, "%d, ", last->line_num);
		fprintf(flog, "%s\n", last->state);
		last = last->next;
		if(last == NULL)
			break;
	}
}

// checking if a label has been repeated 
int LabelNotRepeated(char *pnemonic, char **labels)
{
	int j=0, flag = 0;
	for(j=0;j<label_count_g;j++)
	{
		if(strcmp(pnemonic, labels[j]) == 0)
		{
			flag = 1;
			break;
		}
	}
	return flag;
}

// remove whitespaces
void remove_spaces(char* s) {
    const char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}


int main(int argc, char *argv[])
{
    FILE * fp;
    FILE *flog;
    char name_log[50];
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int NUM_LINES = 100, CHAR_LEN = 100;
    char *lines[NUM_LINES];
    char *lables[NUM_LINES];
    char *pnemonics[NUM_LINES];
    char *operands[NUM_LINES];
    char *exact_labels[NUM_LINES];
    int i, ctr=0;
    for(i=0;i<NUM_LINES;i++)
    {
    	lines[i] = (char*)(malloc(CHAR_LEN*sizeof(char)));
    	lables[i] = (char*)(malloc(CHAR_LEN*sizeof(char)));
    	exact_labels[i] = (char*)(malloc(CHAR_LEN*sizeof(char)));
    	pnemonics[i] = (char*)(malloc(CHAR_LEN*sizeof(char)));
    	operands[i] = (char*)(malloc(CHAR_LEN*sizeof(char)));
    }
    char *file_name = argv[1];
    fp = fopen(file_name, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    strcpy(name_log, file_name);
    strcat(name_log, "_logfile.txt");
    flog = fopen(name_log, "w");
    if(flog == NULL)
	{
	      printf("Error : Cannot open log file");   
	      exit(1);             
	}

    while ((read = getline(&line, &len, fp)) != -1) {
        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);
        int colon = 0;
        char *store = (char*)(malloc(CHAR_LEN*sizeof(char)));
        for(i=0;i<strlen(line);i++)
        {
        	if (line[i] == 10) // newline ascii = 10
        		break;
        	if (line[i] == (int)';')
        		break;
        	if(line[i] == (int)':')
        	{
        		colon = i;
        		break;
        	}
        	store[i]+=line[i];
        }
        if (colon == 0) // colon indicates label for variable or instruction
        {
        	strcpy(lines[ctr], store);
	        strcpy(lables[ctr], "");
        }
        else if (colon != 0)
        {
	        char *processed = (char*)(malloc(CHAR_LEN*sizeof(char)));
	        for(i=colon+1;i<strlen(line);i++)
	        {
	        	if (line[i] == 10)
	        		break;
	        	if (line[i] == (int)';')
	        		break;
	        	if (line[i] == (int)'\t')
	        	{
	        		processed[i-colon-1]+= ' ';
	        		continue;
	        	}
	        	processed[i-colon-1]+=line[i];
	        }
	        strcpy(lines[ctr], processed);
	        strcpy(lables[ctr], store);
    	}
        ctr+=1;
    }
    printf("\nPrinting the lines\n");
    for(i=0;i<ctr;i++)
    {
    	printf("%s\n", lines[i]);
    	char dum[100];
    	strcpy(dum, lines[i]);
    	char* token = strtok(dum, " ");
    	int j = 0;
    	strcpy(pnemonics[i], "");
    	strcpy(operands[i], "");
		while (token) 
		{
			if (j == 0)
			{
				char* c = token;
				while (isspace(*c))
    				++c;
				strcpy(pnemonics[i], c);
				j++;
			}
			else if(j == 1)
			{
				strcpy(operands[i], token);
				j++;
			}
			// for more than one operand 
			else if (j > 1)
			{
				append_error(i+1, "Error : multiple operands");
			}
    		printf("token: %s\n", token);
    		token = strtok(NULL, " ");
		}
    }
    printf("\nPrinting the Pnemonics and Operands\n");
    for(i=0;i<ctr;i++)
    {
    	printf("Pnemonic : %s, Operand : %s\n", pnemonics[i], operands[i]);
    }
    /*printf("\nPrinting and Processing the lables\n");*/
    for(i=0;i<ctr;i++)
    {
    	if (strcmp(exact_labels[i], "")!=0)
    		continue;
    	int j = 0, flag = 0;
    	for(j=0;j<strlen(lables[i]);j++)
    	{
    		if (j == 0 && !isalpha(lables[i][j]))
    		{
    			append_error(i+1, "Error : Invalid label name");
    			flag = 1;
    			break;
    		}
    		if(!isalnum(lables[i][j]))
    		{
    			append_error(i+1, "Error : Invalid label name");
    			flag = 1;
    			break;
    		}
    	}
    	if (strcmp(lables[i], "") != 0 && flag == 0) 
    	{
    		if(LabelNotRepeated(lables[i], exact_labels) == 0)
    		{
	    		if(strcmp(pnemonics[i], "") != 0)
	    		{
	    			strcpy(exact_labels[i], lables[i]);
	    		}
	    		else
	    		{
	    			if (strcmp(lables[i+1], "") == 0)
	    			{
	    				int k = i+1;
	    				while(1)
	    				{
		    				if(strcmp(pnemonics[k], "")!=0)
		    				{
			    				strcpy(exact_labels[i], "");
			    				strcpy(exact_labels[k], lables[i]);
			  					break;
		    				}
		    				k++;
	    				}
	    			}
	    			else
	    			{
	    				append_error(i+1, "Error : conflicting labels");
	    			}

	    		}
    		}
    		else
    		{
    			append_error(i+1, "Error : duplicate label name");
    		}
    	}
    	else
    	{
    		strcpy(exact_labels[i], "");
    	}
    	label_count_g++;
    }
    fclose(fp);
    if (line)
        free(line);
    struct code* head = createCode(pnemonics, operands, exact_labels, ctr);
    displayCodeLL(head);
    printf("Please look at the log file %s for details of the compilation\n", name_log);
    if (error_count_g > 0)
    {
    	fprintf(flog, "Error Detected in asm file. Cannot go ahead!\n");
    	displayErrorLL(flog);
    	exit(EXIT_FAILURE);
    }
    else
    {
    	char* token = strtok(file_name, ".");
    	secondPass(head, file_name);
    	if (error_count_g == 0)
    	{
    		char* token = strtok(file_name, ".");
    		token = strtok(token, "_");
    		fprintf(flog, "Code Compiled Succesfully. Generated %s_listfile.lst and %s_obj.bin files\n", token, token);
    	}
    	else if (error_count_g > 0)
    	{
	    	fprintf(flog, "Error Detected in asm file. Cannot go ahead!\n");
	    	displayErrorLL(flog);
	    	exit(EXIT_FAILURE);    		
    	}
    	exit(EXIT_SUCCESS);
	}
}

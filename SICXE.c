#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int PASS1();
int PASS2();

/*struct OPTAB
{
	char mnemonic[8];
	int opcode;
}optab;*/
	
int main()
{
    PASS1();
    PASS2();
    return 0;
}

int PASS1()
{
	char buffer[32],label[12],mnemonic[8],operand[12],mnem[8],op[2],symbol[12];
	int start, locctr, ret, flag=0, address, j, program_length, count=0, ins_len;
	FILE *fProg,*fSymtab, *fOptab, *fIntrm, *fLength;
	
	fProg = fopen("SICXE_program.txt","r");
	if(fProg == NULL){printf("Source file missing!!"); return 0;}
	
	fSymtab = fopen("SYMTAB.txt","w+");
	
	fOptab = fopen("OPTAB.txt","r");
	if(fOptab == NULL){printf("OPTAB missing!!"); return 0;}
		
	fIntrm = fopen("Intermediate_File.txt","w");
	
	fgets(buffer,32,fProg);
	sscanf(buffer,"%s %s %s",label,mnemonic,operand);
	
	if(strcmp(mnemonic,"START")==0)
	{
		locctr = atoi(operand);	//save operand as starting address
		while(locctr > 0)
		{
			start = start + (locctr%10)*pow(16,count);
			locctr = locctr/10;
			count++;
		}
		locctr = start;
        fprintf(fIntrm,"%X\t%s\t%s\t%s\n",start,label,mnemonic,operand);
	}
	else
	{
		locctr = 0X0;
	}
	
	while(!feof(fProg) || strcmp(label,"END")==0)
	{
		fgets(buffer,32,fProg);
		ret = sscanf(buffer,"%s%s%s",label,mnemonic,operand);
		
		if(label[0] != ';' && label[0] != '.')	//not a comment line
		{	
			if(ret == 1)
			{
				strcpy(mnemonic,label);
				fprintf(fIntrm,"%04X\t\t%s\n",locctr,mnemonic);
			}
			if(ret == 2)
			{
				strcpy(operand,mnemonic);
				strcpy(mnemonic,label);
				fprintf(fIntrm,"%04X\t\t%s\t%s\n",locctr,mnemonic,operand);
			}
			if(ret == 3) //there is a symbol in the Label field
			{
				rewind(fSymtab);		
				while(!feof(fSymtab))
				{
					flag = 0;
					fscanf(fSymtab,"%s%04X",symbol,&address);
					if(strcmp(label,symbol)==0)
					{
						flag = 1;	//duplicate symbol found
						printf("\nDuplicate LABEL found: %s",label);
						return 0;
					}
				}					
				
				if(flag == 0)	//no duplicate symbol
				{
					fprintf(fSymtab,"%s\t%04X\n",label,locctr);
					fprintf(fIntrm,"%04X\t%s\t%s\t%s\n",locctr,label,mnemonic,operand);
				}
			}
			
			rewind(fOptab);
			while(!feof(fOptab))	//search optab for OPCODE
			{
				fscanf(fOptab,"%s%s%d",mnem,op,&ins_len);
				if(strcmp(mnemonic,mnem)==0)
				{
					locctr += ins_len;
					flag = 0;
					break;
				}
				else if(strcmp(mnemonic,"WORD")==0 || strcmp(mnemonic,"word")==0)
				{	
					locctr += 3;
					flag = 0;
					break;
				}
				else if((strcmp(mnemonic,"RESW")==0) || (strcmp(mnemonic,"resw")==0))
				{	
					locctr += 3*atoi(operand);
					flag = 0;
					break;
				}
				else if(strcmp(mnemonic,"RESB")==0 || strcmp(mnemonic,"resb")==0)
				{	
					locctr += atoi(operand);
					flag = 0;
					break;
				}
				else if(strcmp(mnemonic,"BYTE")==0 || strcmp(mnemonic,"byte")==0)
				{
					j = strlen(operand);
					if(operand[0] !='C' && operand[0] != 'X')
					{	locctr += 1;
						flag = 0;
						break;
					}
					else if(strcmp(mnemonic,"BYTE")==0 && operand[0] =='C')
					{
						locctr += j-3;	//-3 is done to account for C' '
						flag = 0;
						break;
					}
					else if(strcmp(mnemonic,"BYTE")==0 && operand[0] =='X')
					{	
						if((j-3)%2 != 0)
							locctr += (j-3)/2 + 1;
						else
							locctr += (j-3)/2 ;
						flag = 0;
						break;
					}
					else if(strcmp(mnemonic,"BASE")==0)
					{
						//locctr will not increment
					}
					else
					{
						flag = 1;
					}
				}
				if(flag == 1)
				{
					printf("\n%s not present in OPTAB!",mnemonic);
					printf("\nExiting ...");
					return 0;
				}
			}
		}
		if(strcmp(mnemonic,"END")==0)
		{
			break;
		}
	}
	printf("\nSYMTAB generated...\n");
	
	fLength = fopen("Program_length.txt","w");
	program_length = locctr - start;
	fprintf(fLength,"%X",program_length);
	
	fclose(fProg);
	fclose(fSymtab);
	fclose(fOptab);
	fclose(fIntrm);
	fclose(fLength);
	return 1;
}

int PASS2()
{
	int locctr, start,program_length,ret,ins_length,address,target,pc,displacement,j,k,d_status,reg1,reg2,ascii,temp1,ins_len, base_Reg = 0X0;
	char label[12],mnemonic[8],operand[12],buffer[32],mnem[8],op[2],opcode[2],symbol[12],immediate_operand[12],cons[8];
	long int a_seek, b_seek;
	FILE *fSymtab, *fOptab, *fIntrm, *fLength, *fobj;
	
	fIntrm = fopen("Intermediate_File.txt","r");
	if(fIntrm == NULL){printf("\nIntermediate file missing!"); return 0;}
    fSymtab=fopen("SYMTAB.txt","r");
    if(fSymtab == NULL){printf("\nSYMTAB missing!"); return 0;}
    fOptab=fopen("OPTAB.txt","r");
    if(fOptab == NULL){printf("\nOPTAB missing!"); return 0;}
    fLength=fopen("Program_length.txt","r");
    if(fLength == NULL){printf("\nProgram_length file missing!"); return 0;}
    fobj = fopen("Object_Program.txt","w");
    
    fscanf(fIntrm,"%X%s%s%s",&locctr,label,mnemonic,operand);
	if(strcmp(mnemonic,"START")==0)
    {
        start = (int)strtol(operand,NULL,16);
        fscanf(fLength,"%X",&program_length);
        fprintf(fobj,"H^%6s^%06X^%06X",label,start,program_length);
    	fprintf(fobj,"\nT^%06X^",start);
	}    
    
	while(!feof(fIntrm))
	{
		//printf("Hi1\t");
		fgets(buffer,32,fIntrm);
		ret = sscanf(buffer,"%X%s%s%s",&locctr,label,mnemonic,operand);
		
		if(ret == 2) //RSUB
		{
			strcpy(mnemonic,label);
		}
		else if(ret == 3)	//label not present
		{
			strcpy(operand,mnemonic);
			strcpy(mnemonic,label);
		}
		else
		{}
		
		rewind(fOptab);
		while(!feof(fOptab))
		{
			//printf("Hi2\t");
			fscanf(fOptab,"%s%s%d",mnem,op,&ins_len);
			if(strcmp(mnemonic,mnem)==0)
			{
				strcpy(opcode,op);
				j = strlen(operand);
				if(ins_len == 3 && ret>2)
				{
					if(operand[j-1]=='X' && operand[j-2]==',')
					{
						if(op[1] < '4'){op[1] = '3';}
						else if(op[1] < '8'){op[1] = '7';}
						else if(op[1] < 'C'){op[1] = 'B';}
						else {op[1] = 'F';}
						
						operand[j-2] = '\0';
						rewind(fSymtab);
						while(!feof(fSymtab))
						{
							//printf("Hi3\t");
							fscanf(fSymtab,"%s%X",symbol,&address);
							if(strcmp(operand,symbol)==0)
							{
								target = address;
								b_seek = ftell(fIntrm);
								fscanf(fIntrm,"%X",&pc);
								a_seek = ftell(fIntrm);
								fseek(fIntrm,a_seek-b_seek,SEEK_END);
								displacement = target - pc;
								d_status = 1;
								if((displacement/0X1000)<0X8)	//pow(16,3)=4096
								{
									displacement += 0X8000;
								}
								break;
							}
						}
					}
					else if (operand[0] == '@')
					{
						d_status = 0;
						if(op[1] < '4'){op[1] = '2';}
						else if(op[1] < '8'){op[1] = '6';}
						else if(op[1] < 'C'){op[1] = 'A';}
						else {op[1] = 'E';}
					}
					else if (operand[0] == '#')
					{
						for(k=1;k<strlen(operand);k++)
						{
							immediate_operand[k-1] = operand[k];
						}
						
						if(strcmp(mnemonic,"LDB")==0)
						{
							rewind(fSymtab);
							while(!feof(fSymtab))
							{
								fscanf(fSymtab,"%s%X",symbol,&address);
								if(strcmp(immediate_operand,symbol)==0)
								{
									base_Reg = strtol(immediate_operand,NULL,16);
									break;
								}
							}
						}
						if(op[1] < '2'){op[1] = '1';}
						else if(op[1] < '4'){op[1] = '3';}
						else if(op[1] < '6'){op[1] = '5';}
						else if(op[1] < '8'){op[1] = '7';}
						else if(op[1] < 'A'){op[1] = '9';}	
						else if(op[1] < 'C'){op[1] = 'B';}
						else if(op[1] < 'E'){op[1] = 'D';}									
						else {op[1] = 'F';}
						immediate_operand[strlen(immediate_operand)]= '\0';
						displacement = strtol(immediate_operand,NULL,16);
						d_status = 1;
					}
					else	//Normal operand
					{
						d_status = 0;
						if(op[1] < '4'){op[1] = '3';}
						else if(op[1] < '8'){op[1] = '7';}
						else if(op[1] < 'C'){op[1] = 'B';}
						else {op[1] = 'F';}
					}
					if (d_status != 1)	//pc relative addressing
					{
						rewind(fSymtab);
						while(!feof(fSymtab))
						{
							//printf("Hi4\t");
							fscanf(fSymtab,"%s%X",symbol,&address);
							if(strcmp(operand,symbol)==0)
							{
								target = address;
								b_seek = ftell(fIntrm);
								fscanf(fIntrm,"%X",&pc);
								a_seek = ftell(fIntrm);
								fseek(fIntrm,-(a_seek-b_seek),1);
								displacement = target - pc;
								
								if(displacement < 0XF800 || displacement > 0X7FF)	//0XF800 means -2048, 0X7FF means 2047
								{
									displacement = target - base_Reg;
								}
								break;
							}
						}
					}
					//Print the objectcode to file
					fprintf(fobj,"%s%04X^",opcode,displacement);
					break;
				}
									
				else if(ins_len == 2)
				{
					j = strlen(operand);
					
					if(operand[0] = 'A'){reg1 = 0;}
					else if(operand[0] = 'X'){reg1 = 1;reg2 = 0;}
					else if(operand[0] = 'L'){reg1 = 2;reg2 = 0;}
					else if(operand[0] = 'B'){reg1 = 3;reg2 = 0;}
					else if(operand[0] = 'S'){reg1 = 4;reg2 = 0;}
					else if(operand[0] = 'T'){reg1 = 5;reg2 = 0;}
					else {reg1 = 6;reg2 = 0;}
			
					if(j > 1)
					{
						if(operand[3] = 'A'){reg2 = 0;}
						else if(operand[3] = 'X'){reg2 = 1;}
						else if(operand[3] = 'L'){reg2 = 2;}
						else if(operand[3] = 'B'){reg2 = 3;}
						else if(operand[3] = 'S'){reg2 = 4;}
						else if(operand[3] = 'T'){reg2 = 5;}
						else {reg2 = 6;}
					}
					d_status = 1;
					//Print the objectcode to file
					fprintf(fobj,"%s%d%d^",opcode,reg1,reg2);
					break;				
				}
				
				else	//ins_len == 4
				{
					rewind(fSymtab);
					while(!feof(fSymtab))
					{
						printf("Hi5\t");
						fscanf(fSymtab,"%s%X",symbol,&address);
						if(strcmp(operand,symbol)==0)
						{
							target = address;
							break;
						}
					}
					target += 0X10000;
					fprintf(fobj,"%s%6X^",opcode,target);
					break;
				}
			}
			
			else if((strcmp(mnemonic,"BYTE")==0) || ((strcmp(mnemonic,"BYTE")==0)))
			{
				if(operand[0] == 'C')
				{
					for(k=0;k<strlen(operand)-3;k++)
					{
						temp1=0x0;
						temp1=temp1+(int)operand[k+2];
						ascii=ascii* 0x100 + temp1;
					}			
					fprintf(fobj,"%6X^",ascii);
					break;
				}
				else	//strcmp(operand[0] == 'X'
				{
					for(k=0;k<strlen(operand)-3;k++)
					{
						cons[k]=operand[k+2];
					}   
					cons[k]='\0';
					fprintf(fobj,"%s^",cons);
					break;
				}
			}
			else if((strcmp(mnemonic,"WORD")==0) || (strcmp(mnemonic,"word")==0))
			{
				temp1 = (int)strtol(operand,NULL,10);
				fprintf(fobj,"%06X^",temp1);
				break;
			}
			else if((strcmp(mnemonic,"RSUB")==0))
			{
				fprintf(fobj,"4C0000^");
				break;
			}
			else	//RESB or RESW
			{
				break;
			}
		}
	}
	fprintf(fobj,"\nE^%06X",start);
	printf("\nObject Program written successfully!\n");
	fclose(fIntrm);
	fclose(fSymtab);
	fclose(fOptab);
	fclose(fLength);
	fclose(fobj);
	return 1;
}		

#include "sis.h"
#include "pld_int.h"  /* added Feb 9 1992 */
#include "ite_int.h"
#include <stdio.h>

		
static struct ite_terminal {
		node_t	*fanin;
		int	node_num;
		ite_vertex	*ite_node;
		}	ite_inode[100];


	
ite_vertex *ite_alloc()
{
ite_vertex *temp;

temp=ALLOC(ite_vertex,1);
temp->IF=NULL;
temp->THEN=NULL;
temp->ELSE=NULL;
temp->multiple_fo=0;
temp->mark=0;
temp->index_size = 0;
temp->print_mark=0;
temp->fo=ALLOC(fanout_ite,1);
temp->fo->parent_ite_ptr=NULL;
temp->fo->next=NULL;
return(temp);
}

/* Rajeev commented it */
/*
void alloc_ite_node(node)
node_t *node;
{
ite_vertex *ite;
(node)->ite_slot=(char *) ite_alloc();
ite_connect(&(node->ite_slot),node->ite_slot);
ite=ite_get(node);
}
*/
	
void
ite_assgn_num(ite, n_ptr,addr_max_stored)
ite_vertex *ite;
int *n_ptr;
int *addr_max_stored;
{

if(ite->mark != 0)
	return;
if(ite->value==2)
	{

	/*store fanin*/
	if(ite->phase==1)
		{
			ite_inode[*addr_max_stored].fanin=(ite->fanin);
			ite_inode[*addr_max_stored].node_num= *n_ptr;	
			(*addr_max_stored)++;
		}
	(ite->mark)=(*n_ptr);
	(*n_ptr)++;
	}
else    {
	(ite->mark)= (*n_ptr);
	(*n_ptr)++;
	if(ite->IF!=NULL)
		{
		ite_assgn_num(ite->IF,n_ptr,addr_max_stored);
       		ite_assgn_num(ite->THEN,n_ptr,addr_max_stored);
        	ite_assgn_num(ite->ELSE,n_ptr,addr_max_stored);
		}
	}
}


/* print out a numbered ITE */
void
ite_print(ite,addr_max_stored)
int *addr_max_stored;
ite_vertex *ite;
{
int found,i;
if((ite->print_mark)==1)
	return;
if(0<=ite->value&&ite->value<=2)
	{
	if(ite->value==2)
		{	
		if(ite->phase==0)
			{
			found=0;
			for(i=0;i<*addr_max_stored;i++)
				{
				if(ite->fanin==ite_inode[i].fanin)
					{
					/*printf("[%d]=[%d]'\n",ite->mark,ite_inode[i].node_num);*/
					printf("[%d]=%s'\n",ite->mark,node_long_name(ite_inode[i].fanin));
					(ite->print_mark)=1;
					found=1;
					break;
					}
				}
			if(found==0)
				{
				 /*printf("[%d]'\n",ite->mark);*/
				 printf("[%d]'=%s'\n",ite->mark,node_long_name(ite->fanin));
				(ite->print_mark)=1;
				}
			}
		else    {
			/*printf("[%d]\n",ite->mark);*/
 			printf("[%d]=%s\n",ite->mark,node_long_name(ite->fanin)); 
			/* printf("[%d]=%s\n",ite->mark,ite->name); */ /* Rajeev */
			(ite->print_mark)=1;
			}
		}
	else if(ite->value==1)
		{
		printf("[%d]=1\n",ite->mark);
		(ite->print_mark)=1;
		}
	else 
		{
		printf("[%d]=0\n",ite->mark);
		(ite->print_mark)=1;
		}
	return;
	}
else    {
	printf("[%d]=[%d, %d, %d], cost = %d, pattern_num = %d\n",
               ite->mark, ite->IF->mark, ite->THEN->mark, ite->ELSE->mark, 
               ite->cost, ite->pattern_num);		
	(ite->print_mark)=1;
	ite_print(ite->IF,addr_max_stored);
	ite_print(ite->THEN,addr_max_stored);
	ite_print(ite->ELSE,addr_max_stored);
	}
}

			
/* print ite */

void
ite_print_dag(ite)
ite_vertex *ite;
{
int node_num, max_stored;

max_stored=0;
node_num=1;
ite_assgn_num(ite, &node_num, &max_stored);
ite_print(ite, &max_stored);
ite_clear_dag(ite);
}

void ite_clear_dag(ite)
ite_vertex *ite;
{
if(ite->mark==0)
	return;
ite->mark=0;
ite->print_mark=0;
if(ite->IF!=NULL){
	ite_clear_dag(ite->IF);
	ite_clear_dag(ite->THEN);
	ite_clear_dag(ite->ELSE);
}
}
void
ite_print_out(ite)
ite_vertex *ite;
{
int node_num, max_stored;

max_stored=0;
node_num=1;
ite_assgn_num(ite, &node_num,&max_stored);
ite_print(ite,&max_stored);
ite_clear_dag(ite);
}
/* commented Jul 21, 1992 to see if Will's files can be ignored */
/*
#include "ite_scite"
#include "ite_share"
#include "ite_canonical"
*/ 

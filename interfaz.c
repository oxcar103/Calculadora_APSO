#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <signal.h>

#include "comunes.h"

int op_seguir = 0;

void menu(float *num1, float *num2, int *operacion){	
	if(op_seguir == 0){
		printf("Introduce un numero: ");
		scanf("%f", num1);
	}
	
	printf("Operacion\n");
	printf("1. Suma\n");
	printf("2. Resta\n");
	printf("3. Multiplicacion\n");
	printf("4. Division\n");
	printf("5. Salir\n");
	do{
		printf("Selecciona operacion: ");
		scanf("%d", operacion);
	}while(*operacion < 1 || *operacion > 5);
	
	if(*operacion != 5){
		do{
			printf("Introduce otro numero: ");
			scanf("%f", num2);
		}while(*num2 == 0 && *operacion == 4);
	}
}

int main(){
	float resultado; 
	int pid_op1, pid_op2, pid_operador;
	int fifo_motor;
	key_t clave_cola;
	int id_cola;
	char sig_operacion;
	
	struct operacion op;
	struct numero num1, num2;
	
	//Creamos la cola para pasar los parametros
	clave_cola=ftok("./Makefile", 1024);
	id_cola=msgget(clave_cola, 0600 | IPC_CREAT);
	
	
	//Creamos el proceso hijo OP_1
	pid_op1=fork();
	if(pid_op1==0){
		execl("OP_1", "OP_1", NULL);
	}
	
	//Creamos el proceso hijo OP_2
	pid_op2=fork();
	if(pid_op2==0){
		execl("OP_2", "OP_2", NULL);
	}

	//Creamos el proceso hijo operador
	pid_operador=fork();
	if(pid_operador==0){
		execl("operador", "operador", NULL);
	}
	
	//Creamos la fifo del motor
	mkfifo("fifo_motor", 0777);
	mkfifo("fifo_op2", 0777);
	mkfifo("fifo_operador", 0777);

	op.opcion = 1;	
	
	do{	
		//Indicamos el tipo de los parametros
		op.tipo=3;
		num1.tipo=1;
		num2.tipo=2;
		
		//Obtenemos los parametros
		menu(&num1.num, &num2.num, &op.opcion);
		
		if(op.opcion != 5){
			
			op_seguir = 1;

			//Pasamos el primer operando al proceso OP_1
			msgsnd(id_cola, (struct msgbuf *) &num1, sizeof(num1) - sizeof(long), 0);
			
			//Pasamos el operador al proceso operador
			msgsnd(id_cola, (struct msgbuf *) &op, sizeof(op) - sizeof(long), 0);

			//Pasamos el segundo operando al proceso OP_2
			msgsnd(id_cola, (struct msgbuf *) &num2, sizeof(num2) - sizeof(long), 0);
			
			
			//Leemos el resultado del motor
			fifo_motor=open("fifo_motor", O_RDWR);
			read(fifo_motor, &resultado, sizeof(resultado));
			
			switch(op.opcion){
				case 1:
					sig_operacion = '+';
				break;
				
				case 2:
					sig_operacion = '-';
				break;
				
				case 3:
					sig_operacion = 'x';
				break;
				
				case 4:
					sig_operacion = '/';
				break;
			}
			
			//printf("Resultado: %f\n\n", resultado);
			printf("%f %c %f = %f\n\n", num1.num, sig_operacion, num2.num, resultado);
			
			//copiamos el resultado en op1
			num1.num=resultado;
		}
		
		
	}while(op.opcion != 5);
	
	kill(pid_op1, 32);
	kill(pid_op2, 32);
	kill(pid_operador, 32);
	
	close(fifo_motor);
	unlink("fifo_motor");
	unlink("fifo_op2");
	unlink("fifo_operador");
	msgctl(id_cola, IPC_RMID, 0);
	
	//Parche para matar al proceso motor
	system("killall motor");
	
	return resultado;
}
			

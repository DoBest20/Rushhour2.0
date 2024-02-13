#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define SIZE 1024
#define ERROR -2
#define HASH_SIZE 2000

typedef enum {
	// start,
	left,
	right,
	up,
	down,
	quit,
	N_op 
} commands ;

typedef enum {
	vertical,
	horizontal
} direction ;

typedef struct {
	int id ;
	int y1, y2 ;	// y1: the minimum of y, y2: the maximum of y
	int x1, x2 ;	// x1: the minimum of x, x2: the maximum of x
	int span ;		// the number of cells 
	direction dir ;	// the direction of the car
} car_t ;

typedef struct hash_node{ 
	car_t * cars;
	int pre_idx;
	int my_idx;
	int pre_key;
	int key;
	struct hash_node * next;
} state;

typedef struct{
	state * head;
}Table;

Table hash_table[1000] = {0};

car_t * route_path[500] = {0};
int route_count= 0;

typedef struct QNode
{
    state * data;
    struct QNode * next;
    struct QNode * prev;
}QNode;

typedef struct MyQueue
{
    struct QNode * front;
    struct QNode * rear;
    int size;
}MyQueue;



int n_cars = 0 ;
// car_t * cars = 0x0 ; //모든 차의 정보를 담고 있어야 함, 그 정보들을 가리키는 포인터 
int cells[6][6] ; // cells[Y][X] 안에 car_t의 id를 담고 있어야 함

state * possible_move (state * s, int id, int op);
int is_overlap(car_t * cars, int id, int op);
int is_dup_state(state * s, MyQueue * q, state *V, int idx);
void display ();
int update_cells (car_t * cars2);


void save_route(state * s)
{
	while(s->pre_key != -1)
	{
		route_path[route_count] = s->cars;
		route_count++;
		if(s->pre_idx == 0)
		{
			s = hash_table[s->pre_key].head;
			
		}
		else
		{
			int n = s->pre_idx;
			s = hash_table[s->pre_key].head;
			for(int i=0; i < n; i++)
			{
				s = s->next;
			}
		}
	}
		route_path[route_count] = s->cars;

}

int hash_function(car_t * cars)
{  
    int hash_key = 0;

    for(int i=1; i<=n_cars; i++)
    {
        hash_key += (cars[i].x1 + cars[i].x2) * (i+2) * cars[i].dir;
        hash_key += (cars[i].y1 + cars[i].y2) * (i+2) * ((cars[i].dir + 1) % 2) * 10;
    }
    hash_key = hash_key % 1000;

    return hash_key;
}

int save_hash_table(state * s)
{
	int hash_key = s->key;

	if(hash_table[hash_key].head == NULL)
	{
		hash_table[hash_key].head = s;
		return 0;
	}
	else
	{
		int count = 0;
		// int check_dup = 0;
		state * temp = hash_table[hash_key].head;
		while(1)
		{
			
			count++;
			if(memcmp(temp->cars, s->cars, sizeof(car_t) * (n_cars + 1)) == 0)
			{
				// check_dup = 1;
				free(s);
				// fprintf(stderr,"same state");
				return 1;
			}
			
			if(temp->next == NULL)
			{
				// fprintf(stderr,"not same\n");
				break;
			}
			temp = temp->next;
		}	
		// if(check_dup == 0)
		// {
			s->my_idx = count;
			temp->next = s;
		// }
	}
	return 0;
}

QNode * getQNode(state * data, QNode * prev)
{
    QNode * ref = (QNode * ) malloc(sizeof(QNode));
    if (ref == NULL)
    {
        return NULL;
    }
    ref->data = data;
    ref->next = NULL;
    ref->prev = prev;
    return ref;
}

MyQueue * getMyQueue()
{
    MyQueue * ref = (MyQueue * ) malloc(sizeof(MyQueue));
    if (ref == NULL)
    {
        return NULL;
    }
    ref->front = NULL;
    ref->rear = NULL;
    return ref;
}

void enqueue(MyQueue * ref, state * data)
{
    QNode * node = getQNode(data, ref->rear);
    if (ref->front == NULL)
    {
        ref->front = node;
        ref->size = 1;
    }
    else
    {
        ref->rear->next = node;
        ref->size = ref->size + 1;
    }
    ref->rear = node;
}

int isEmpty(MyQueue * ref)
{
    if (ref->size == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

state * peek(MyQueue * ref)
{
    if (isEmpty(ref) == 1)
    {
        printf("\n Empty Queue");
		state * tmp = malloc (sizeof(state));
		tmp -> pre_key = ERROR;
        // When stack is empty
        return tmp;
    }
    else
    {
        return ref->front->data;
    }
}

state * dequeue(MyQueue * ref)
{
    if (isEmpty(ref) == 1)
    {
		state * tmp = malloc(sizeof(state));
		tmp->pre_key = ERROR;

        return tmp;
    }
    else
    {
        state * data = peek(ref);
        QNode * temp = ref->front;
        if (ref->front == ref->rear)
        {
            ref->rear = NULL;
            ref->front = NULL;
        }
        else
        {
            ref->front = ref->front->next;
            ref->front->prev = NULL;
        }
        ref->size--;
        return data;
    }
}

state * load_game (char * filename)
{
	FILE *inputF;
	char *line = NULL;
	size_t size = 0;
	ssize_t read = 0;
	int id = 1;
	int j; // sscanf 사용 
	state * s = malloc(sizeof(state));

	s->my_idx = 0;
	s->next = NULL;
	s->pre_idx = -1;
	s->pre_key = ERROR ;
	
	if((inputF = fopen(filename, "r")) == NULL){
		fclose(inputF);
		return s;
	} // 해당 파일이 없는 경우

	getline(&line, &size, inputF);
	
	if(sscanf(line, "%d", &j) == 0){
		return s;
	}

	n_cars = atoi(line);
	if( n_cars <2 || n_cars >37){
		return s;
	} 

	
	s->cars = (car_t*)malloc(sizeof(car_t) * (n_cars+1));

	while((read = getline(&line, &size, inputF)) != -1){
		if(id > n_cars) {
			break;
		}
		char * ptr = strtok(line, ":");
		
		if(ptr == NULL){
			return s;
		}

		if('A' <= ptr[0] && ptr[0] <='F' && ptr[1] >='1' && ptr[1] <='6'){
			if(id == 1){ // 시작 차는 4번째 줄
				if(ptr[1] != '4'){
					return s;
				}
			}
			s->cars[id].x1 = ptr[0] - 'A';
			s->cars[id].x2 = s->cars[id].x1;
			s->cars[id].y1 = ptr[1] - '1';
			s->cars[id].y2 = s->cars[id].y1;
			//시작 위치값 설정 
			
		}
		else{
			return s;
		}

		char * dir = strtok(NULL, ":");

		if(dir == NULL){
			return s;
		}

		if(strcmp(dir, "vertical") == 0)
			s->cars[id].dir = vertical;
		else if(strcmp(dir, "horizontal") == 0)
			s->cars[id].dir = horizontal;
		else{
			return s;
		}
		

		char * span = strtok(NULL, ":");
		if(span == NULL){
			return s;
		}

		if(sscanf(span, "%d", &j) == 0){//정수인지 확인 
			return s;
		}

		if(atoi(span) <= 0 && atoi(span) >= 7){ //1~6사이의 숫자가 아니라면 
			return s;
		}
		s->cars[id].span = atoi(span);

		if(s->cars[id].dir == horizontal){// horizontal
			s->cars[id].x2 = s->cars[id].x1 + s->cars[id].span -1;
			if(s->cars[id].x2 >5){
				return s;
			}
		}
		else {//vertical
			s->cars[id].y1 = s->cars[id].y2 - s->cars[id].span +1;
			if(s->cars[id].y1 <0){
				return s;
			}
		}
		id++;
	}
	
    
	if(id-1!= n_cars){
		return s;
	} //만약 개수가 맞지 않다면 

	free(line);
	fclose(inputF);

	// for(int i=1; i<=n_cars; i++){
	// 	fprintf(stderr, "car %d x1 = %d, y1 = %d, x2 = %d, y2 = %d\n", i+1, s->cars[i].x1, s->cars[i].y1, s->cars[i].x2, s->cars[i].y2);
	// }

	s->pre_key = -1;
	return s;
}

int is_dup_state(state * s, MyQueue * q, state *V, int idx){
	// for(node * i = q->front; i != NULL; i = i->next){
	// 	if(memcpy(s.cars, i->s.cars, sizeof(s.cars)) == 0)
	// 		return 1;
	// }
	for (int j=0; j<idx; j++){
		if(memcpy(s->cars, V[j].cars, sizeof(s->cars)) == 0){
			return 1;
		}
	}
	return 0;
}

int find_Adj(state * s, MyQueue * q){
	for(int i=1; i <= n_cars; i++){
		for (int j=0; j<4; j++){
			state * adj_s = possible_move(s, i, j);
			if(adj_s->pre_key == ERROR){
				continue;
			}

			// fprintf(stderr, "here!!\n");
			adj_s->key = hash_function(adj_s->cars);
			int dup = save_hash_table(adj_s);
			// fprintf(stderr, "here!!!!!!!!!!!!!!!!!!\n");
			if ( dup != 1){
				// fprintf(stderr, "%d\n", adj_s->key);
				update_cells(adj_s->cars);
				// display();
				if(cells[3][5] == 1){
					printf("WIN!\n");
					save_route(adj_s);
					return 1;

				}
				enqueue(q, adj_s);
				
			}
		}
	}
	return 0;
	// fprintf(stderr, "find adj finish\n");
}

int find_solution(char* filename){
	MyQueue * q = getMyQueue();

	state * s0;
	// s0->cars = (car_t *) malloc (sizeof(car_t) * (n_cars + 1)); 
	s0 = load_game(filename); // 여기서 이미 malloc 해주고 보내줌 
	if(update_cells(s0->cars) == 1){
		fprintf(stderr, "Invalid Data\n");
		return 1;
	}
	display();

	if(s0->pre_key == ERROR){
		fprintf(stderr, "load game error\n");
		return 1;
	}
	
	s0->key = hash_function(s0->cars);
	save_hash_table(s0);
	enqueue(q,s0);

	// fprintf(stderr, "queue size : %d\n", q->size);
	while(q->size > 0){
		state * s = dequeue(q);

		if( find_Adj(s, q) == 1)
			break;
	}
	printf("finish\n");
	return 0;
}

void display ()
{
	for(int i=5; i>=0; i--){
		for (int j=0; j<=5; j++){
			if(cells[i][j] == 0){
				printf("%4s", "+");
			}
			else{
				printf("%4d", cells[i][j]);
			}
		}
		printf("\n");
	}
	printf("\n\n");
}

int update_cells (car_t * cars2)
{
	memset(cells, 0, sizeof(int) * 36) ; // clear cells before the write.

	//FIXME
	// return 0 for sucess
	// return 1 if the given car information (cars) has a problem

	for(int i=1; i<=n_cars; i++){
		for(int a = cars2[i].y1;a<=cars2[i].y2;a++){
			for (int b= cars2[i].x1; b<=cars2[i].x2; b++){
				if(cells[a][b] == 0){
					cells[a][b] = i;
				}
				else {
					return 1;
				}
			}
		}
	}
	return 0;
}

int is_overlap(car_t * cars, int id, int op){
	int target_x;
	int target_y;
	switch (op)
	{
	case left:
		target_x = cars[id].x1-1;
		target_y = cars[id].y1;
		break;
	case right:
		target_x = cars[id].x2+1;
		target_y = cars[id].y1;
		break;
	case up:
		target_x = cars[id].x1;
		target_y = cars[id].y2+1;
		break;
	case down:
		target_x = cars[id].x1;
		target_y = cars[id].y1-1;
		break;
	default:
		break;
	}
	for(int i=1; i<=n_cars; i++){
		if(id == i) {
			continue;
		}
		for (int c = cars[i].x1; c <= cars[i].x2; c++){
			for (int d = cars[i].y1; d <= cars[i].y2; d++){
				if(target_x == c && target_y == d){
					return 1;
				}
			}
		}
	}

	return 0;
}

state * possible_move (state * s, int id, int op) 
{
	state * adj = malloc (sizeof(state));
	adj->cars = (car_t *) malloc (sizeof(car_t) * (n_cars + 1));
	memcpy(adj->cars, s->cars, sizeof(car_t) * (n_cars + 1));
	adj->next = NULL;
	adj->my_idx = 0;
	adj->pre_idx = s->my_idx;
	adj->pre_key = ERROR;
	adj->key = -1;

	if(adj->cars[id].dir == horizontal){
		if(op == left) { // left
			if(adj->cars[id].x1 - 1 >= 0 && is_overlap(adj->cars, id, op) == 0){//board를 벗어나지 않고 이미 차가 존재하지 않는 경우
				adj->cars[id].x1 --;
				adj->cars[id].x2 --;
				adj->pre_key = s->key;
			}
		}
		else if(op == right){
			if(adj->cars[id].x2 + 1 <= 5 && is_overlap(adj->cars, id, op) == 0){//board를 벗어나지 않고 이미 차가 존재하지 않는 경우
				adj->cars[id].x2 ++;
				adj->cars[id].x1 ++;
				adj->pre_key = s->key;
			}
		}
	}
	else if(adj->cars[id].dir == vertical){
		if(op == up){
			if(adj->cars[id].y2 + 1 <= 5 && is_overlap(adj->cars, id, op) == 0){//board를 벗어나지 않고 이미 차가 존재하지 않는 경우
				adj->cars[id].y2 ++;
				adj->cars[id].y1 ++;
				adj->pre_key = s->key;
			}
		}
		else if(op == down){
			if(adj->cars[id].y1 - 1 >= 0 && is_overlap(adj->cars, id, op) == 0){//board를 벗어나지 않고 이미 차가 존재하지 않는 경우
				adj->cars[id].y1 --;
				adj->cars[id].y2 --;
				adj->pre_key = s->key;
			}
		}
	}

	return adj;
}



GtkWidget *create_colored_event_box(int car_id) {
    GtkWidget *event_box = gtk_event_box_new();
    gtk_widget_set_size_request(event_box, 40, 40);
    GdkColor red_color, white_color;
    gdk_color_parse("red", &red_color);
    gdk_color_parse("white", &white_color);

    GdkColor color;

    if (car_id == 0) {
        // 0인 경우 흰색으로 설정
        gdk_color_parse("white", &color);
    }
    else if(car_id == 1){
        gdk_color_parse("red", &color);
    } 
    else {
        do {
            color.red = (car_id * 30) % 256 * 257;
            color.green = (car_id * 40) % 256 * 257;
            color.blue = (car_id * 20) % 256 * 257;
        } while (gdk_color_equal(&color, &white_color) || gdk_color_equal(&color, &red_color));
    }

    gtk_widget_modify_bg(event_box, GTK_STATE_NORMAL, &color);

    return event_box;
}
GtkWidget *window;
GtkWidget *table;
void update_colors() {
    GtkWidget *event_box;
    GdkColor color;

	// cells[0][0] = rand();
	route_count--;
	if(route_count < 0)
	{
		return;
	}
	update_cells(route_path[route_count]);

    // Iterate through the cells and update colors
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {	
            event_box = create_colored_event_box(cells[i][j]);
            gtk_table_attach_defaults(GTK_TABLE(table), event_box, j, j + 1, i, i + 1);
        }
    }
	gtk_widget_show_all(window);
}


int main(int argc, char *argv[]) {
    char buf[128] ;
	if(find_solution("board3.txt") == 1){
		fprintf(stderr,"Invalid Data\n");
	}
	update_cells(route_path[route_count]);
	// display();

	// for(int i=route_count; i>=0; i--)
	// {
		
	// 	update_cells(route_path[i]);
	// 	display();
		
	// }

    GtkWidget *event_box;
    GtkWidget *next_button;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 360);
    gtk_window_set_title(GTK_WINDOW(window), "GtkTable");

    gtk_container_set_border_width(GTK_CONTAINER(window), 5);

    table = gtk_table_new(7, 7, TRUE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table), 2);

    int i, j;
    for (i = 5; i >=0; i--) {
        for (j = 0; j < 6; j++) {
            event_box = create_colored_event_box(cells[i][j]);
            gtk_table_attach_defaults(GTK_TABLE(table), event_box, j, j + 1, i, i + 1);
        }
    }

    // Corner button 추가 (오른쪽 맨 아래)
    next_button = gtk_button_new_with_label("Next");
	g_signal_connect(G_OBJECT(next_button), "clicked", G_CALLBACK(update_colors),NULL);

    gtk_table_attach(GTK_TABLE(table), next_button, 6, 7, 6, 7, GTK_SHRINK, GTK_SHRINK, 0, 0);

    gtk_container_add(GTK_CONTAINER(window), table);

    g_signal_connect(G_OBJECT(window), "destroy",
        G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
	

    gtk_main();
	

	
    return 0;
}


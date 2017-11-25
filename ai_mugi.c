/*
  1. Janssonライブラリをインストール。
      sudo apt-get install libjansson-dev
  2. コンパイル。
      gcc -o sampleAI sampleAI.c -ljansson
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#define BUF_SIZE 4096
#define MAP_SIZE (20)
#define QUEUE_SIZE (MAP_SIZE*MAP_SIZE)
#define FREESPACE (0)
#define BLOCK (1)
#define WALL (2)
#define ITEM (3)
#define EVENT (4)
#define MOVE_FAIL (-1)
#define MOVE_UP (0)
#define MOVE_DOWN (1)
#define MOVE_RIGHT (2)
#define MOVE_LEFT (3)
#define THRESHOLD_HAMMER (8)
#define THRESHOLD_NEAR_ITEM (5)
//#define THRESHOLD_FAR_ITEM (15)
#define THRESHOLD_HEAL_HP (9)
#define THRESHOLD_HEAL_AGI (14)
#define THRESHOLD_HEAL_STR (10)

int array_threshold_far_item[11]={999,20,15,14,13,12,11,8,7,6,6};


char map[MAP_SIZE][MAP_SIZE];


typedef struct queue_element_t{
		int x;
		int y;
		int dir;
		int hammer;
		int count;
} queue_element;

queue_element queue[QUEUE_SIZE];
int queue_rp;
int queue_wp;
int current_weapon_pow=0;
int current_map_level;

int is_heal(json_t *player, float rate)
{
		int result=0;
		json_int_t hp;
		json_int_t maxhp;
		json_int_t heal;
		json_int_t agi;
		json_int_t str;
		hp=json_integer_value(json_object_get(player,"hp"));
		maxhp=json_integer_value(json_object_get(player,"maxhp"));
		heal=json_integer_value(json_object_get(player,"heal"));
		agi=json_integer_value(json_object_get(player,"agi"));
		str=json_integer_value(json_object_get(player,"str"));

		if((heal>0) && 
			( (hp<=(THRESHOLD_HEAL_HP+maxhp/10+current_map_level/10)*rate)
				 || (agi<=THRESHOLD_HEAL_AGI)
				 || (str<=THRESHOLD_HEAL_STR)))
		{
			result=1;
		}
		
		return result;
}

int make_map(json_t* obj, int value)
{
		int i;
		int x,y;
		int array_size;
		json_t *tmp;

		array_size = json_array_size(obj);
		for(i=0;i<array_size;i++)
		{
			tmp=json_array_get(obj,i);
			x=json_integer_value(json_array_get(tmp,0));
			y=json_integer_value(json_array_get(tmp,1));
			map[x][y]=value;
		}
		return array_size;
}

void push(int x, int y, int dir, int hammer,int count)
{
		queue[queue_wp].x=x;
		queue[queue_wp].y=y;
		queue[queue_wp].dir=dir;
		queue[queue_wp].hammer=hammer;
		queue[queue_wp].count=count;
		queue_wp=(queue_wp+1)%QUEUE_SIZE;
}

int pop(int *x, int *y, int *dir, int *hammer,int *count)
{
		if(queue_wp==queue_rp)
			return 0;
		
		*x=queue[queue_rp].x;
		*y=queue[queue_rp].y;
		*dir=queue[queue_rp].dir;
		*hammer=queue[queue_rp].hammer;
		*count=queue[queue_rp].count;
		queue_rp=(queue_rp+1)%QUEUE_SIZE;
		return 1;
}

int search(int x, int y, int target,int hammer, int *count)
{
		int find=0;
		int dir;
		int used_hammer;
		int step;
		char tmp_map[MAP_SIZE][MAP_SIZE];
		queue_rp=0;
		queue_wp=0;
		memcpy(tmp_map,map,MAP_SIZE*MAP_SIZE);
		push(x,y,-1,hammer,0);

		while(pop(&x,&y,&dir,&hammer,&step))
		{
//				printf("x:%d y:%d dir:%d tmp_map:%d \n",x,y,dir,tmp_map[x][y]);

				if(tmp_map[x][y]==target)
				{
					find=1;
					*count=step;
					break;
				}
				tmp_map[x][y]=WALL;
				used_hammer=0;
				if(tmp_map[x-1][y]==FREESPACE || tmp_map[x-1][y]==target || ( tmp_map[x-1][y]==BLOCK && hammer>0))
				{
//						printf("PUSH LEFT\n");
						if(tmp_map[x-1][y]==BLOCK)
						{
								used_hammer=1;
						}
						else
						{
								used_hammer=0;
						}
						if(dir==-1)
						{
								push(x-1,y,MOVE_LEFT,hammer-used_hammer,step+1);
						}
						else
						{
								push(x-1,y,dir,hammer-used_hammer,step+1);
						}

				}
				if(tmp_map[x+1][y]==FREESPACE || tmp_map[x+1][y]==target|| ( tmp_map[x+1][y]==BLOCK && hammer>0))
				{
//						printf("PUSH RIGHT\n");

						if(tmp_map[x+1][y]==BLOCK)
						{
								used_hammer=1;
						}
						else
						{
								used_hammer=0;
						}
						if(dir==-1)
						{
								push(x+1,y,MOVE_RIGHT,hammer-used_hammer,step+1);
						}
						else
						{
								push(x+1,y,dir,hammer-used_hammer,step+1);
						}
				}
				if(tmp_map[x][y-1]==FREESPACE || tmp_map[x][y-1]==target|| ( tmp_map[x][y-1]==BLOCK && hammer>0))
				{
//						printf("PUSH UP\n");

						if(tmp_map[x][y-1]==BLOCK)
						{
								used_hammer=1;
						}
						else
						{
								used_hammer=0;
						}
						if(dir==-1)
						{
								push(x,y-1,MOVE_UP,hammer-used_hammer,step+1);
						}
						else
						{
								push(x,y-1,dir,hammer-used_hammer,step+1);
						}
				}
				if(tmp_map[x][y+1]==FREESPACE || tmp_map[x][y+1]==target|| ( tmp_map[x][y+1]==BLOCK && hammer>0))
				{
//						printf("PUSH DOWN\n");

						if(tmp_map[x][y+1]==BLOCK)
						{
								used_hammer=1;
						}
						else
						{
								used_hammer=0;
						}
						if(dir==-1)
						{
								push(x,y+1,MOVE_DOWN,hammer-used_hammer,step+1);
						}
						else
						{
								push(x,y+1,dir,hammer-used_hammer,step+1);
						}
				}
		}
		if(find==1)
			return dir;
		else
			return MOVE_FAIL;
}

int next_move(json_t* message)
{
		json_t* player;
		json_t* cur_pos;
		int x,y;
		int num_items;
		int num_hammer;
		int num_heal;
		int level;
		int map_level;
		int leveling=0;
		int ret=MOVE_FAIL;
		int ret_used_hammer;
		int count=0;
		int count_used_hammer;
		int candidation;
		memset(map,0,MAP_SIZE*MAP_SIZE);
	
		make_map(json_object_get(message,"blocks"),BLOCK);
		make_map(json_object_get(message,"walls"),WALL);
		num_items=make_map(json_object_get(message,"items"),ITEM);
		make_map(json_object_get(message,"kaidan"),EVENT);
		make_map(json_object_get(message,"boss"),EVENT);
		make_map(json_object_get(message,"ha2"),EVENT);
		make_map(json_object_get(message,"events"),EVENT);
	
		player=json_object_get(message,"player");
		cur_pos=json_object_get(player,"pos");
		x=json_integer_value(json_object_get(cur_pos,"x"));
		y=json_integer_value(json_object_get(cur_pos,"y"));

		num_hammer=json_integer_value(json_object_get(player,"hammer"));
		num_heal=json_integer_value(json_object_get(player,"heal"));
		map_level=json_integer_value(json_object_get(player,"map-level"));
		level=json_integer_value(json_object_get(player,"level"));

		current_map_level=map_level;

#if 0
		if((num_hammer>0) && ((map_level%5==0) || (num_heal<=1)) )
		{
				//破壊にゃんこ
				ret=search(x,y,BLOCK,num_hammer,&count);
		}
#endif

		if((num_items>0) && (ret==MOVE_FAIL))
		{
				//お宝ハンターにゃんこ
				ret=search(x,y,ITEM,0,&count);
				if(num_hammer>0)
				{
						ret_used_hammer=search(x,y,ITEM,1,&count_used_hammer);
						if(count_used_hammer+THRESHOLD_HAMMER<count)
						{
								ret=ret_used_hammer;
								count=count_used_hammer;
						}
				}
				if((num_heal==0) && (count>=THRESHOLD_NEAR_ITEM))
				{
						ret=MOVE_FAIL;
				}
		}

#if 1
		if((ret==MOVE_FAIL) && (num_heal>=4) && (map_level==6 || map_level==13) && (level<map_level) &&(current_weapon_pow>map_level*1.0))
		{
				//レベリングにゃんこ
				leveling=1;
				if(map[x-1][y]==FREESPACE)
						ret=MOVE_LEFT;
				else if(map[x+1][y]==FREESPACE)
						ret=MOVE_RIGHT;
				else if(map[x][y-1]==FREESPACE)
						ret=MOVE_UP;
				else if(map[x][y+1]==FREESPACE)
						ret=MOVE_DOWN;
		}
#endif
		if(((ret==MOVE_FAIL) || (count>=array_threshold_far_item[map_level/10])) && (leveling==0))
		{
				//逃亡にゃんこ
				ret=search(x,y,EVENT,0,&count);
				if(num_hammer>0)
				{
						ret_used_hammer=search(x,y,EVENT,1,&count_used_hammer);
						if(count_used_hammer+THRESHOLD_HAMMER<count)
						{
								ret=ret_used_hammer;
						}
				}
		}

		return ret;
}

void map_mode(json_t* message)
{
		json_t *player;
		int ret;
    const char* choices[] = {"UP", "DOWN", "RIGHT", "LEFT"};
    int nchoices = sizeof(choices)/sizeof(choices[0]);

		player=json_object_get(message,"player");

//		printf("hp:%lld maxhp:%lld\n",hp,max_hp);
		if(is_heal(player,1))
		{
				puts("HEAL");
		}
		else
		{
				ret=next_move(message);
//				printf("ret:%d\n",ret);
				if(ret!=MOVE_FAIL)
				{
						
						printf("%s\n",choices[ret]);
				}
				else
				{
					  puts(choices[rand() % nchoices]);
				}
		}
}

void battle_mode(json_t* message)
{
		int i;
		json_t *player;
		json_t *monster;
		json_t *monsters;
		json_int_t minhp[2]={65536,65536};
		json_int_t tmp;
		json_int_t str;
		int target[2]={1,1};
		int num_monsters;
		int alive_monsters=0;
		player=json_object_get(message,"player");
		monsters=json_object_get(message,"monsters");
		num_monsters = json_array_size(monsters);
//		printf("%d\n",num_monsters);
		str=json_integer_value(json_object_get(player,"str"));
		

		for(i=0;i<num_monsters;i++)
		{
			monster=json_array_get(monsters,i);
			tmp=json_integer_value(json_object_get(monster,"hp"));
			if(tmp>0)
			{
				alive_monsters++;
				if((minhp[1]>tmp))
				{
						target[1]=json_integer_value(json_object_get(monster,"number"));
						minhp[1]=tmp;
				}
				if((minhp[0]>minhp[1]))
				{
						tmp=minhp[0];
						minhp[0]=minhp[1];
						minhp[1]=tmp;
						tmp=target[0];
						target[0]=target[1];
						target[1]=tmp;
				}
			}
		}
		if(is_heal(player,1+(alive_monsters*0.1)))
		{
				puts("HEAL");
		}
		else
		{
				if(alive_monsters>=2)
				{
						if(minhp[0]<str/5)
						{
								printf("DOUBLE %d %d\n",target[0],target[1]);
						}
						else
						{
								printf("STAB %d\n",target[0]);
						}
				}
				else
				{
				    printf("STAB %d\n",target[0]);
				}
		}
}

void equip_mode(json_t* message)
{
		json_int_t sum_now;
		json_int_t sum_discover;
		json_t *now;
		json_t *discover;

    now = json_object_get(message, "now");
		discover = json_object_get(message, "discover");

		sum_now=json_integer_value(json_object_get(now,"str"))
						+json_integer_value(json_object_get(now,"hp"))
						+json_integer_value(json_object_get(now,"agi"));
		

		sum_discover=json_integer_value(json_object_get(discover,"str"))
						+json_integer_value(json_object_get(discover,"hp"))
						+json_integer_value(json_object_get(discover,"agi"));

		if(json_string_value(json_object_get(now,"name"))=="猫の爪")
		{
				//にゃんこボーナス発動
				sum_now++;
		}
		if(sum_now<sum_discover)
		{
				puts("YES");
				current_weapon_pow=sum_discover;
		}
		else
		{
				puts("NO");
		}

#if 0
		printf("%s %lld %lld %lld\n"
		,json_string_value(json_object_get(now,"name"))
		,json_integer_value(json_object_get(now,"str"))
		,json_integer_value(json_object_get(now,"hp"))
		,json_integer_value(json_object_get(now,"agi")));
		printf("%s %lld %lld %lld\n"
		,json_string_value(json_object_get(discover,"name"))
		,json_integer_value(json_object_get(discover,"str"))
		,json_integer_value(json_object_get(discover,"hp"))
		,json_integer_value(json_object_get(discover,"agi")));
#endif
}

void levelup_mode(json_t* message)
{
		json_t *player;
		json_int_t maxhp;
		json_int_t maxstr;
		json_int_t maxagi;
		player=json_object_get(message,"player");
		maxhp=json_integer_value(json_object_get(player,"maxhp"));
		maxstr=json_integer_value(json_object_get(player,"maxstr"));
		maxagi=json_integer_value(json_object_get(player,"maxagi"));

		if(maxhp<(maxstr+maxagi))
		{
				//健康第一
		    puts("HP");
		}
		else if(maxstr<maxagi)
		{
				//力 is power
		    puts("STR");
		}
		else
		{
		    puts("AGI");
		}

}

int main()
{
    // 標準出力を行バッファリングにする。
    setvbuf(stdout, NULL, _IOLBF, 0);

    // 名を名乗る。 Your name.
    printf("ROBO CAT MUGI\n");

    char line[BUF_SIZE];
    while (fgets(line, BUF_SIZE, stdin) != NULL) {
        json_t *message = json_loads(line, 0, NULL);

        if (!message || !json_is_object(message))
            abort();

        if (json_object_get(message, "map"))
            map_mode(message);
        else if (json_object_get(message, "battle"))
            battle_mode(message);
        else if (json_object_get(message, "equip"))
            equip_mode(message);
        else if (json_object_get(message, "levelup"))
            levelup_mode(message);
        else
            abort();

        json_decref(message);
    }
    return 0;
}

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

char map[MAP_SIZE][MAP_SIZE];


typedef struct queue_element_t{
		int x;
		int y;
		int dir;
} queue_element;

queue_element queue[QUEUE_SIZE];
int queue_rp;
int queue_wp;


int is_heal(json_t *player, int threshold)
{
		int result=0;
		json_int_t hp;
		json_int_t maxhp;
		json_int_t heal;
		hp=json_integer_value(json_object_get(player,"hp"));
		maxhp=json_integer_value(json_object_get(player,"maxhp"));
		heal=json_integer_value(json_object_get(player,"heal"));

		if((heal>0) && ( (hp<=10) ||(hp<(maxhp/threshold))))
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

void push(int x, int y, int dir)
{
		queue[queue_wp].x=x;
		queue[queue_wp].y=y;
		queue[queue_wp].dir=dir;
		queue_wp=(queue_wp+1)%QUEUE_SIZE;
}

int pop(int *x, int *y, int *dir)
{
		if(queue_wp==queue_rp)
			return 0;
		
		*x=queue[queue_rp].x;
		*y=queue[queue_rp].y;
		*dir=queue[queue_rp].dir;
		queue_rp=(queue_rp+1)%QUEUE_SIZE;
		return 1;
}

int search(int x, int y, int target)
{
		int find=0;
		int dir;
		char tmp_map[MAP_SIZE][MAP_SIZE];
		queue_rp=0;
		queue_wp=0;
		memcpy(tmp_map,map,MAP_SIZE*MAP_SIZE);
		push(x,y,-1);
		
		while(pop(&x,&y,&dir))
		{
//				printf("x:%d y:%d dir:%d tmp_map:%d \n",x,y,dir,tmp_map[x][y]);

				if(tmp_map[x][y]==target)
				{
					find=1;
					break;
				}
				tmp_map[x][y]=WALL;
				if(tmp_map[x-1][y]==FREESPACE || tmp_map[x-1][y]==target)
				{
//						printf("PUSH LEFT\n");
						if(dir==-1)
						{
								push(x-1,y,MOVE_LEFT);
						}
						else
						{
								push(x-1,y,dir);
						}
				}
				if(tmp_map[x+1][y]==FREESPACE || tmp_map[x+1][y]==target)
				{
//						printf("PUSH RIGHT\n");
						if(dir==-1)
						{
								push(x+1,y,MOVE_RIGHT);
						}
						else
						{
								push(x+1,y,dir);
						}
				}
				if(tmp_map[x][y-1]==FREESPACE || tmp_map[x][y-1]==target)
				{
//						printf("PUSH UP\n");

						if(dir==-1)
						{
								push(x,y-1,MOVE_UP);
						}
						else
						{
								push(x,y-1,dir);
						}
				}
				if(tmp_map[x][y+1]==FREESPACE || tmp_map[x][y+1]==target)
				{
//						printf("PUSH DOWN\n");

						if(dir==-1)
						{
								push(x,y+1,MOVE_DOWN);
						}
						else
						{
								push(x,y+1,dir);
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

		if((num_hammer>0) && ((map_level%5==0) || (num_heal<=1)) )
		{
				//破壊にゃんこ
				ret=search(x,y,BLOCK);
		}

		if((num_items>0) && (ret==MOVE_FAIL) && (num_heal>=1))
		{
				//お宝ハンターにゃんこ
				ret=search(x,y,ITEM);
		}

#if 0
		if((ret==MOVE_FAIL) &&(num_heal>(map_level/7)+1) && ((map_level%7)==6) && (level<map_level))
		{
				//レベリングにゃんこ
				leveling=1;
				ret=MOVE_FAIL;
		}
#endif
		if((ret==MOVE_FAIL) && (leveling==0))
		{
				//逃亡にゃんこ
				ret=search(x,y,EVENT);
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
		if(is_heal(player,3))
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
		if(alive_monsters>4 && is_heal(player,2))
		{
				puts("HEAL");
		}

		else if(is_heal(player,3))
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
				puts("YES");
		else
				puts("NO");

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

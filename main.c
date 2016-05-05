#include  <unistd.h>
#include  <sys/types.h>       /* basic system data types */
#include  <sys/socket.h>      /* basic socket definitions */
#include  <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>       /* inet(3) functions */
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <errno.h>
#include <hiredis/hiredis.h>
#include "cJSON/cJSON.h"
#include "unidirectional_queue/unidirectional_queue.h"

/*
 * Todo
 * take your char control function in to a library,
 * there is to many char deal function in the main.
 * */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <pthread.h>
#include <strings.h>
#include <stdbool.h>


const char *not_chinese_pattern = "^[^\u4e00-\u9fa5]++";

//typedef struct sockaddr  SA;
const int8_t chinese_word_byte = sizeof("字") - 1;
const int8_t chinese_char_byte = sizeof("字");
const char *redis_get_common = "get ";
struct TREE_CUT_WORDS_STUFF
{
    char *start_word_item;
    bool have_first_not_cn_word;
    char *first_not_cn_word;
    char *had_deal_search_word;
};

/*
 * the parent tree function copy its not finished participle result
 * and send the result to the children fucntion will use this struct
 * */
struct TREE_CUT_WORDS_DATA
{
    //participle result,may be finished,may be not.
    char *cut_word_result;
    //the original pointer for the remain search words.use for free.
    char *origin_remainder_words;
    //the pointer for the remain search,it will go head in the code deal.
    char *remainder_words;
    /*
     * The weight of the participle result,we will have many participle result.
     * The weight larger,the result better.
     * */
    int *words_weight;

    //This LinkQueue is use to store the finished participle result of all the children thread
    LinkQueue *pullwords_result_queue;
    //as it say,The lock is use for pullwords_result_queue writing.
    pthread_mutex_t *pullwords_result_queue_write_lock;
    /*
     * When all the children thread finish their participle,We need notice parent thread that.
     * pullwords_cond is use for this
     * */
    pthread_cond_t *pullwords_cond;
    //count the participle result number that need finished.
    int *pullwords_result_number;
    //count the participle result number that had finished
    int *pullwords_result_finish_number;
};

char *substring(char *ch, int pos, int length)
{
    char *pch = ch;
    char *subch = (char *) calloc(sizeof(char), length + 1);
    int i;
    pch = pch + pos;
    for (i = 0; i < length; i++) {
        subch[i] = *(pch++);
    }
    subch[length] = '\0';
    return subch;
}

//initialization the struct TREE_CUT_WORDS_DATA variable
void init_tree_cut_words_data(struct TREE_CUT_WORDS_DATA *cut_words_data)
{
    (*cut_words_data).cut_word_result = (char *) malloc(WORD_RESULT_LEN);
    memset((*cut_words_data).cut_word_result, 0, WORD_RESULT_LEN);
    (*cut_words_data).words_weight = (int *) malloc(sizeof(int));
    memset((*cut_words_data).words_weight, 0, sizeof(int));
    (*cut_words_data).remainder_words = (char *) malloc(MAX_SEARCH_WORDS_LEN);
    memset((*cut_words_data).remainder_words, 0, MAX_SEARCH_WORDS_LEN);
    (*cut_words_data).origin_remainder_words = (*cut_words_data).remainder_words;
}

//get the search words form http request.
void get_search_words(char *http_request, char *search_wrods)
{
    char search_words_key[20] = "search_words";
    char *search_words_position;
    char *search_words_origin_position;
    search_words_position = strstr(http_request, search_words_key);
    search_words_origin_position = search_words_position =
            search_words_position + strlen(search_words_key) + strlen("=");
    int search_wrods_lenght;
    for (int i = 0; i < MAX_SEARCH_WORDS_LEN; i++) {
        search_words_position++;
        if ((*search_words_position) == ' ') {
            search_wrods_lenght = i + 1;
            break;
        }
    };
    strncpy(search_wrods, search_words_origin_position, search_wrods_lenght);
}

//result weight add fucntion
void add_words_weight(int *weight, int type)
{
    switch (type) {
        case 1:
            *weight += 5;
            break;
        case 2:
            *weight += 10;
            break;
    }
}

//left cut string
char *left(char *dst, char *src, int n)
{
    char *p = src;
    char *q = dst;
    int len = strlen(src);
    if (n > len) n = len;
    while (n--) *(q++) = *(p++);
    *(q++) = '\0';
    return dst;
}

char *join_char(const char *a, const char *b)
{
    char *c = (char *) malloc(strlen(a) + strlen(b) + 1);
    memset(c, 0, strlen(a) + strlen(b) + 1);
    if (c == NULL) exit(1);
    char *tempc = c; //store the header point;
    while (*a != '\0') {
        *c++ = *a++;
    }
    while ((*c++ = *b++) != '\0') { ;
    }
    return tempc;//tempc need free
}

/*
 * return 1: success
 * return 0: error;
 * */
int8_t pecl_regx_match(PCRE2_SPTR subject, PCRE2_SPTR pattern, int *match_offset, int *match_str_lenght)
{

    pcre2_code *re;
    PCRE2_SPTR name_table;

    int crlf_is_newline;
    int errornumber;
    int i;
    int namecount;
    int name_entry_size;
    int rc;
    int utf8;

    uint32_t option_bits;
    uint32_t newline;

    PCRE2_SIZE erroroffset;
    PCRE2_SIZE *ovector;

    size_t subject_length;
    pcre2_match_data *match_data;

    char *_pattern = "[^\u4e00-\u9fa5]++";

    subject_length = strlen((char *) subject);

    re = pcre2_compile(
            pattern,               /* the pattern */
            PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
            0,                     /* default options */
            &errornumber,          /* for error number */
            &erroroffset,          /* for error offset */
            NULL);                 /* use default compile context */

    if (re == NULL) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        printf("PCRE2 compilation failed at offset %d: %s\n", (int) erroroffset,
               buffer);
        return 0;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    rc = pcre2_match(
            re,                   /* the compiled pattern */
            subject,              /* the subject string */
            subject_length,       /* the length of the subject */
            0,                    /* start at offset 0 in the subject */
            0,                    /* default options */
            match_data,           /* block for storing the result */
            NULL);                /* use default match context */
    if (rc < 0) {
        switch (rc) {
            case PCRE2_ERROR_NOMATCH:
                break;
                /*
                Handle other special cases if you like
                */
            default:
                printf("Matching error %d\n", rc);
                break;
        }
        pcre2_match_data_free(match_data);   /* Release memory used for the match */
        pcre2_code_free(re);                 /* data and the compiled pattern. */
        return 0;
    }
    ovector = pcre2_get_ovector_pointer(match_data);
    *match_offset = (int) ovector[0];


    if (rc == 0) {
        printf("ovector was not big enough for all the captured substrings\n");
    }
    for (i = 0; i < rc; i++) {
        size_t substring_length = ovector[2 * i + 1] - ovector[2 * i];
        *match_str_lenght = (int) substring_length;


        PCRE2_SPTR substring_start = subject + ovector[2 * i];

        if (i > 1) {
            //wrong. i can't large than one.todo.
        }
    }
}

//add a word item to tree participle result
void add_word_item_to_single_tree_result(char *tree_result, const char *word_item)
{
    strcat(tree_result, ",");
    strcat(tree_result, word_item);
}

//just add the chinese words that in dictionary to participle result; and add weight.
void chinese_word_cut(char *tree_result_word, char **search_word_point, int *words_weight, const char *chinese_words) {
    strcat(tree_result_word, ",");
    strcat(tree_result_word, chinese_words);
    add_words_weight(words_weight, 2);
    //the point go ahead
    *search_word_point = *search_word_point + strlen(chinese_words);
}

/*
 * If the remain words go to the end,we will use this fucntion.
 * */
void not_match_word_end_cut(char *tree_result_word, char **search_word_point, int *words_weight)
{
    strcat(tree_result_word, ",");
    strcat(tree_result_word, *search_word_point);
    add_words_weight(words_weight, 1);
    int remain_word_lenght = (int) strlen(*search_word_point);
    //remain search word point go ahead.
    *search_word_point = *search_word_point + remain_word_lenght;
}


void get_redis_connect(redisContext **redis_connect)
{
    *redis_connect = redisConnect("127.0.0.1", 6379);
    if (redis_connect == NULL || (*redis_connect)->err) {
        if (redis_connect) {
            printf("Error: %s\n", (*redis_connect)->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
}


/*
 * deal the not_chinese_word, cut it and add it to the participle result as a words item.
 * */
int not_chinese_word_cut(char *tree_result_word, char **search_word_point, int *words_weight)
{
    int not_chinese_match_offset, not_chinese_match_str_lenght;
    int pecl_regx_match_result = pecl_regx_match((PCRE2_SPTR) (*search_word_point), (PCRE2_SPTR) not_chinese_pattern,
                                                 &not_chinese_match_offset,
                                                 &not_chinese_match_str_lenght);

    /*
     * match that not chinese
     * */
    if (pecl_regx_match_result > 0) {
        add_words_weight(words_weight, 2);
        //取出非中文
        char not_chinese_word[not_chinese_match_str_lenght + 1];
        memset(not_chinese_word, 0, not_chinese_match_str_lenght + 1);
        strncpy(not_chinese_word, *search_word_point, not_chinese_match_str_lenght);
        strcat(not_chinese_word, "\0");
        add_word_item_to_single_tree_result(tree_result_word, not_chinese_word);
        *search_word_point =
                *search_word_point + not_chinese_match_str_lenght;

    }
    return 1;
}

//send http header
void sendHeader(char *Status_code, char *Content_Type, int TotalSize, int socket)
{
    char *head = "\r\nHTTP/1.1 ";
    char *content_head = "\r\nContent-Type: ";
    char *server_head = "\r\nServer: PT06";
    char *length_head = "\r\nContent-Length: ";
    char *date_head = "\r\nDate: ";
    char *newline = "\r\n";
    char contentLength[100];

    time_t rawtime;

    time(&rawtime);

    // int contentLength = strlen(HTML);
    sprintf(contentLength, "%i", TotalSize);

    char *message = malloc((
                                   strlen(head) +
                                   strlen(content_head) +
                                   strlen(server_head) +
                                   strlen(length_head) +
                                   strlen(date_head) +
                                   strlen(newline) +
                                   strlen(Status_code) +
                                   strlen(Content_Type) +
                                   strlen(contentLength) +
                                   28 +
                                   sizeof(char)) * 2);

    if (message != NULL) {

        strcpy(message, head);

        strcat(message, Status_code);

        strcat(message, content_head);
        strcat(message, Content_Type);
        strcat(message, server_head);
        strcat(message, length_head);
        strcat(message, contentLength);
        strcat(message, date_head);
        strcat(message, (char *) ctime(&rawtime));
        strcat(message, newline);

        sendString(message, socket);

        free(message);
    }
}

/*
 * The function may product many thread to deal the participle.
 * I use recursion
 * */
int _tree_cut_words(struct TREE_CUT_WORDS_DATA *parent_cut_words_data)
{

    not_chinese_word_cut((*parent_cut_words_data).cut_word_result, &((*parent_cut_words_data).remainder_words),
                     (*parent_cut_words_data).words_weight);

    char *first_chinese_word;
    first_chinese_word = (char *) malloc(chinese_char_byte);
    memset(first_chinese_word, 0, chinese_char_byte);
    if (!first_chinese_word) {
        //todo.deal the error,not just printf.
        printf("Not Enough Memory!/n");
    }

    strncpy(first_chinese_word, parent_cut_words_data->remainder_words, chinese_word_byte);
    strcat(first_chinese_word, "\0");
    char *get_word_item_cmd = join_char(redis_get_common, first_chinese_word);

    //free Memory
    free(first_chinese_word);

    redisContext *redis_connect;
    get_redis_connect(&redis_connect);

    redisReply *redis_reply = redisCommand(redis_connect, get_word_item_cmd);
    char *match_words_json_str = redis_reply->str;
    free(get_word_item_cmd);
    cJSON *match_words_json = cJSON_Parse(match_words_json_str);

    //The variable is check the remain words is the end or not
    bool NEED_not_match_word_end_cut;

    /*
     * count the number,how many words items that in dictionary is also a path in the remain words.
     * */
    int8_t match_words_item_number = 0;

    //check that, if one or more word item in dictionary has this fhinese chinese word.
    if (redis_reply->type == REDIS_REPLY_STRING) {
        for (int tree_arg_index = 0; tree_arg_index < cJSON_GetArraySize(match_words_json); tree_arg_index++) {
            cJSON *subitem = cJSON_GetArrayItem(match_words_json, tree_arg_index);
            //check the valuestring is in the remain words or not.
            if (strstr(parent_cut_words_data->remainder_words, subitem->valuestring) != NULL) {
                match_words_item_number++;
            }
        };
    } else {
        NEED_not_match_word_end_cut = false;
    }


    if (match_words_item_number == 0) {
        NEED_not_match_word_end_cut = true;
    }

    if (NEED_not_match_word_end_cut == false) {
        int8_t add_result_number = 0;
        add_result_number = match_words_item_number - 1;
        __sync_fetch_and_add((*parent_cut_words_data).pullwords_result_number, add_result_number);
        for (int tree_arg_index = 0; tree_arg_index < cJSON_GetArraySize(match_words_json); tree_arg_index++) {
            cJSON *subitem = cJSON_GetArrayItem(match_words_json, tree_arg_index);
            //check the valuestring is in the remain words or not.
            if (strstr(parent_cut_words_data->remainder_words, subitem->valuestring) != NULL) {

                /*
                 * copy the participle result.send it to the children thread.
                 * */
                struct TREE_CUT_WORDS_DATA *tree_cut_words_data = (struct TREE_CUT_WORDS_DATA *) malloc(
                        sizeof(struct TREE_CUT_WORDS_DATA));
                memset(tree_cut_words_data, 0, sizeof(struct TREE_CUT_WORDS_DATA));
                init_tree_cut_words_data(tree_cut_words_data);

                strcpy(tree_cut_words_data->cut_word_result, parent_cut_words_data->cut_word_result);
                strcpy(tree_cut_words_data->remainder_words, parent_cut_words_data->remainder_words);
                *tree_cut_words_data->words_weight = *(parent_cut_words_data->words_weight);

                //todo,include the five point to one point.
                tree_cut_words_data->pullwords_result_queue = parent_cut_words_data->pullwords_result_queue;
                tree_cut_words_data->pullwords_result_queue_write_lock = parent_cut_words_data->pullwords_result_queue_write_lock;
                tree_cut_words_data->pullwords_cond = parent_cut_words_data->pullwords_cond;
                (*tree_cut_words_data).pullwords_result_number = (*parent_cut_words_data).pullwords_result_number;
                (*tree_cut_words_data).pullwords_result_finish_number = (*parent_cut_words_data).pullwords_result_finish_number;

                chinese_word_cut(tree_cut_words_data->cut_word_result, &(tree_cut_words_data->remainder_words),
                                 tree_cut_words_data->words_weight, subitem->valuestring);

                //not finish,create another thread to deal with it.
                if (strlen(tree_cut_words_data->remainder_words) > 0) {
                    pthread_t pthread_id;
                    int pthread_create_result;
                    pthread_create_result = pthread_create(&pthread_id, NULL,
                                                           (void *) _tree_cut_words, tree_cut_words_data);
                    if (pthread_create_result != 0) {
                        //todo,deal the error,.
                        printf("Create pthread error!\n");
                    }
                } else {
                    //participle is finished.
                    //create the last participle result;
                    struct TREE_CUT_WORDS_RESULT *last_tree_cut_words_result = (struct TREE_CUT_WORDS_RESULT *) malloc(
                            sizeof(struct TREE_CUT_WORDS_RESULT));
                    memset(last_tree_cut_words_result, 0, sizeof(struct TREE_CUT_WORDS_RESULT));
                    strcpy(last_tree_cut_words_result->pullwords_result, tree_cut_words_data->cut_word_result);
                    last_tree_cut_words_result->words_weight = *(tree_cut_words_data->words_weight);

                    //lock
                    pthread_mutex_lock(tree_cut_words_data->pullwords_result_queue_write_lock);
                    //write the result to the queue.
                    EnQueue((*tree_cut_words_data).pullwords_result_queue, last_tree_cut_words_result);
                    *((*parent_cut_words_data).pullwords_result_finish_number) =
                            (*((*parent_cut_words_data).pullwords_result_finish_number)) + 1;

                    //campare the need finish and had finish number. If equal.notify the first parent thread.
                    if (*((*parent_cut_words_data).pullwords_result_finish_number) ==
                        *((*parent_cut_words_data).pullwords_result_number)) {
                        pthread_cond_signal((*parent_cut_words_data).pullwords_cond);
                    }
                    pthread_mutex_unlock(tree_cut_words_data->pullwords_result_queue_write_lock);
                }
            }
        }
    } else {
        /*
         * use the first chinese word to find the word item in dictionary.If find nothing,It is end.finish
         * If find some word items,but no one in the remain words,It is end.finish
         * */
        not_match_word_end_cut(parent_cut_words_data->cut_word_result, &(parent_cut_words_data->remainder_words),
                               parent_cut_words_data->words_weight);
        //create the last participle result;
        struct TREE_CUT_WORDS_RESULT *last_tree_cut_words_result = (struct TREE_CUT_WORDS_RESULT *) malloc(
                sizeof(struct TREE_CUT_WORDS_RESULT));
        memset(last_tree_cut_words_result, 0, sizeof(struct TREE_CUT_WORDS_RESULT));
        strcpy(last_tree_cut_words_result->pullwords_result, parent_cut_words_data->cut_word_result);
        last_tree_cut_words_result->words_weight = *(parent_cut_words_data->words_weight);

        //lock
        pthread_mutex_lock(parent_cut_words_data->pullwords_result_queue_write_lock);
        //write the result to the queue.
        EnQueue((*parent_cut_words_data).pullwords_result_queue, last_tree_cut_words_result);
        *((*parent_cut_words_data).pullwords_result_finish_number) =
                (*((*parent_cut_words_data).pullwords_result_finish_number)) + 1;
        //campare the need finish and had finish number. If equal.notify the first parent thread.
        if (*((*parent_cut_words_data).pullwords_result_finish_number) ==
            *((*parent_cut_words_data).pullwords_result_number)) {
            pthread_cond_signal((*parent_cut_words_data).pullwords_cond);
        }
        pthread_mutex_unlock(parent_cut_words_data->pullwords_result_queue_write_lock);
    }

    //free the data
    free(parent_cut_words_data->cut_word_result);
    free(parent_cut_words_data->words_weight);
    free(parent_cut_words_data->origin_remainder_words);
    free(parent_cut_words_data);
    return 0;
}

/*
 * get the best result from the queue
 * */
void deal_pullwords_result_queue(LinkQueue *pullwords_result_queue, char *result_words)
{
    QueuePtr p;
    p = (*pullwords_result_queue).front->next;
    if (p == NULL) {
        printf("QueuePtr ERROR \n");
    }

    char temporary_result_words[MAX_SEARCH_WORDS_LEN];
    memset(temporary_result_words, 0, MAX_SEARCH_WORDS_LEN);
    int temporary_words_weight = 0;
    while (p != NULL) {//遍历队
        if (p->data->words_weight > temporary_words_weight) {
            temporary_words_weight = p->data->words_weight;
            strcpy(temporary_result_words, p->data->pullwords_result);
        }
        p = p->next;
    }
    char *not_finish_result_words = substring(temporary_result_words, 1, sizeof(temporary_result_words) - 1);
    strcpy(result_words, not_finish_result_words);
    free(not_finish_result_words);
}


int pull_word(char *origin_search_words, int *connfd) {

    pthread_mutex_t pull_word_cond_mtx = PTHREAD_MUTEX_INITIALIZER;
    //use to notify.
    pthread_cond_t pullwords_cond = PTHREAD_COND_INITIALIZER;

    char option;
    LinkQueue *pullwords_result_queue = (LinkQueue *) malloc(sizeof(LinkQueue));
    memset(pullwords_result_queue, 0, sizeof(LinkQueue));
    InitQueue(pullwords_result_queue);
    //lock for queue writing
    pthread_mutex_t pullwords_result_queue_write_lock = PTHREAD_MUTEX_INITIALIZER;

    //the first parent_tree_cut_words_data
    struct TREE_CUT_WORDS_DATA *parent_tree_cut_words_data = (struct TREE_CUT_WORDS_DATA *) malloc(
            sizeof(struct TREE_CUT_WORDS_DATA));
    init_tree_cut_words_data(parent_tree_cut_words_data);
    //todo，make this code in a function.start
    parent_tree_cut_words_data->pullwords_result_queue = pullwords_result_queue;
    parent_tree_cut_words_data->pullwords_result_queue_write_lock = &pullwords_result_queue_write_lock;
    parent_tree_cut_words_data->pullwords_cond = &pullwords_cond;
    (*parent_tree_cut_words_data).pullwords_result_number = (int *) malloc(sizeof(int8_t));
    memset((*parent_tree_cut_words_data).pullwords_result_number, 0, sizeof(int8_t));
    (*parent_tree_cut_words_data).pullwords_result_finish_number = (int *) malloc(sizeof(int8_t));
    memset((*parent_tree_cut_words_data).pullwords_result_finish_number, 0, sizeof(int8_t));
    *((*parent_tree_cut_words_data).pullwords_result_number) = 1;
    *((*parent_tree_cut_words_data).pullwords_result_finish_number) = 0;
    //todo，make this code in a function.end

    strcpy(parent_tree_cut_words_data->remainder_words, origin_search_words);

    //lock
    pthread_mutex_lock(&pull_word_cond_mtx);

    //create a new thread to deal this http request,
    int pthread_create_result;
    pthread_t parent_thread_id;
    pthread_create_result = pthread_create(&parent_thread_id, NULL,
                                           (void *) _tree_cut_words, parent_tree_cut_words_data);

    //If pullwords_result_number isnot equal to pullwords_result_finish_number,hang-up and wait.
    while (*((*parent_tree_cut_words_data).pullwords_result_number) !=
           *((*parent_tree_cut_words_data).pullwords_result_finish_number)) {
        //hang-up and wait.
        pthread_cond_wait(&pullwords_cond, &pull_word_cond_mtx);
    }

    char last_result_words[WORD_RESULT_LEN];
    memset(last_result_words, 0, WORD_RESULT_LEN);
    //check the queue and get the best participle result.put it in "last_result_words"
    deal_pullwords_result_queue(pullwords_result_queue, last_result_words);
     strcat(last_result_words,"\n");
    printf("last result_words is %s\n", last_result_words);
    free(pullwords_result_queue);

    //sendString(last_result_words, connfd);
    //sendHeader("200 OK", "text/html; charset=utf-8",MAX_SEARCH_WORDS_LEN, *connfd);

    sendString(last_result_words, *connfd);

    //write(*connfd, last_result_words, sizeof(last_result_words)); //write maybe fail,here don't process failed error

    free(origin_search_words);
    close(*connfd);
    return 0;
};

void deal_pullwords_request(int *connfd)
{

    size_t n;
    char client_request_data[HTTP_REQUEST_LEN];
    memset(client_request_data, 0, HTTP_REQUEST_LEN);

    n = read(*connfd, client_request_data, HTTP_REQUEST_LEN);

    if (n < 0) {
        if (errno != EINTR) {
            perror("read error");
        }
    }
    if (n == 0) {
        //connfd is closed by client
        close(*connfd);
        printf("client exit\n");
    }
    //client exit
    if (strncmp("exit", client_request_data, 4) == 0) {
        close(*connfd);
        printf("client exit\n");
    }

    char *search_words = (char *) malloc(MAX_SEARCH_WORDS_LEN);
    memset(search_words, 0, MAX_SEARCH_WORDS_LEN);
    get_search_words(client_request_data, search_words);
    printf("client input search_words %s\n", search_words);

    //strcpy(search_words, "iphone");
    pull_word(search_words, &(*connfd));

}

//send string to client socket.
sendString(char *message, int socket)
{
    int length, bytes_sent;
    length = strlen(message);

    bytes_sent = send(socket, message, length, 0);

    return bytes_sent;
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    int serverPort = 9999;
    int listenq = 1024;
    pid_t childpid;
    char buf[MAX_SEARCH_WORDS_LEN];
    socklen_t socklen;

    struct sockaddr_in cliaddr, servaddr;
    socklen = sizeof(cliaddr);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serverPort);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket error");
        return -1;
    }

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));


    if (bind(listenfd, (struct sockaddr *) &servaddr, socklen) < 0) {
        perror("bind error");
        return -1;
    }
    if (listen(listenfd, listenq) < 0) {
        perror("listen error");
        return -1;
    }

    printf("pullwords server startup,listen on port:%d\n", serverPort);
    for (; ;) {
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &socklen);
        if (connfd < 0) {
            perror("accept error");
            continue;
        }

        sprintf(buf, "accept form %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);


        //create a thread to deal a http request.
        pthread_t pthread_id;
        int pthread_create_result;
        pthread_create_result = pthread_create(&pthread_id, NULL,
                                               (void *) deal_pullwords_request, &connfd);
        if (pthread_create_result != 0) {
            printf("pthread_create error.\n");
        }

    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

size_t NUM_OR_LINES = 20;

bool OutputWords (int num_of_words, char ** words_buffer, int WORD_TO_REVERSE_IDX);
void OutputReverseWord(char *word_for_reverse);
char ** FillPtrBuff(int num_of_words, char *string_buffer);
int SepOnWords(char *input_str);


int main(int argc, char * argv[])
{
    if (argc != 2) 
    {
        printf("Incorrect input\n");
        return -1;
    }

    int WORD_TO_REVERSE_IDX = atoi(argv[1]);
    printf("WORD_TO_REVERSE_IDX = %d\n", WORD_TO_REVERSE_IDX);

    bool if_word_idx_correct = false; 
    size_t string_size = 0;
    ssize_t chars_num = 0;

    size_t lines_num = 0;

    char ** lines_buff = (char **) calloc (NUM_OR_LINES, sizeof(char *));
    lines_buff

    for (int i = 0; i < NUM_OR_LINES: i++) 
        chars_num = getline(lines_buff[i], &string_size, stdin);
    
    while ((chars_num = getline(&string_buffer, &string_size, stdin)) != -1 && strcmp("!", string_buffer))
    {
        int num_of_words = SepOnWords(string_buffer);
        printf("f\n");
        char **words_buffer = FillPtrBuff(num_of_words, string_buffer);
        assert(words_buffer);
        for (int i = 0; i < num_of_words; i++)
        {
            assert(words_buffer[i]);
            printf("%d) Word: %s\n", i, words_buffer[i]);
        }

        if (~(if_word_idx_correct = OutputWords(num_of_words, words_buffer, WORD_TO_REVERSE_IDX)))
            return -1;

        free(string_buffer);
        free(words_buffer);
    }


    return 0;
}

bool OutputWords (int num_of_words, char ** words_buffer, int WORD_TO_REVERSE_IDX)
{
    assert(words_buffer);

    if (num_of_words <= WORD_TO_REVERSE_IDX)
    {
        printf("num of words is less than %d\n", WORD_TO_REVERSE_IDX);
        return false;
    }

    for (int i = num_of_words - 1; i >= 0; i--)
    {
        if (i == WORD_TO_REVERSE_IDX) 
        {
            OutputReverseWord(words_buffer[i]);
            continue;
        }

        printf("%s", words_buffer[i]); 
    }

    return true;
}

void OutputReverseWord(char *word_for_reverse)
{
    assert(word_for_reverse);

    size_t word_len = strlen(word_for_reverse);
    for (int i = word_len - 1; i >= 0; i--)
    {
        putchar(word_for_reverse[i]);
    }

    putchar('\n');
}

char ** FillPtrBuff(int num_of_words, char *string_buffer)
{
    assert(string_buffer);

    char **words_buffer = (char **)calloc(num_of_words, sizeof(char *));

    for (int i = 0; i < num_of_words; i++)
    {
        words_buffer[i] = string_buffer;
        string_buffer = strchr(string_buffer, '\0') + 1;
    }

    return words_buffer;
}

int SepOnWords(char *input_str)
{
    assert(input_str);

    while (*input_str == ' ')
        input_str++;
    if (*input_str == '\0') return 0;

    int num_of_words = 0;
    char *space = NULL;

    while (space = strchr(input_str, ' '))
    {
        *space = '\0';
        num_of_words++;
        input_str = ++space;
        while (*input_str == ' ') input_str++;
    }
    num_of_words++;

    return num_of_words;
}
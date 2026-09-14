#include <stdio.h>
#include <ctype.h>

#define MAX_NODES 26
#define MAX_INPUT 256

/* Only output values are stored here; this is not a tree structure. */
typedef struct {
    int nodeCount;
    int leafCount;
    int nonLeafCount;
    int height;
    int degree;
    int hasC;
    char parentC;
    char childrenC[MAX_NODES + 1];
    int childCountC;
} TreeInfo;

typedef enum {
    EXPECT_NODE,
    AFTER_NODE,
    AFTER_SUBTREE
} ParseState;

/* Read one line. Spaces and tabs around tokens are permitted. */
int readExpression(char expression[])
{
    int ch;
    int length = 0;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (isspace((unsigned char)ch))
            continue;

        if (ch == '\0' || length >= MAX_INPUT - 1)
            return 0;

        expression[length++] = (char)ch;
    }

    expression[length] = '\0';
    return 1;
}

int errorMessage(const char *message)
{
    printf("오류: %s\n", message);
    return 0;
}

/* Validate and calculate directly from the input, without building a tree. */
int analyzeTree(const char expression[], TreeInfo *info)
{
    char nodeStack[MAX_NODES];
    int counterStack[MAX_NODES];
    int used[MAX_NODES] = {0};
    int top = -1;
    int cLevel = -1;
    char lastNode = '\0';
    ParseState state = EXPECT_NODE;
    int i;

    *info = (TreeInfo){0};

    if (expression[0] == '\0')
        return errorMessage("입력된 트리가 없습니다.");

    if (expression[0] != 'A')
        return errorMessage("루트 노드는 A여야 합니다.");

    for (i = 0; expression[i] != '\0'; i++) {
        char ch = expression[i];

        if (ch >= 'A' && ch <= 'Z') {
            int depth = top + 1;

            if (state != EXPECT_NODE)
                return errorMessage("노드 앞에 쉼표가 없거나 루트가 여러 개입니다.");

            if (used[ch - 'A'])
                return errorMessage("같은 노드 이름을 두 번 사용할 수 없습니다.");

            used[ch - 'A'] = 1;
            info->nodeCount++;

            if (top >= 0) {
                /* Count a child only for its immediate parent. */
                counterStack[top]++;

                /* Use C's counter stack entry to record its direct children. */
                if (top == cLevel) {
                    int childIndex = counterStack[top] - 1;
                    info->childrenC[childIndex] = ch;
                    info->childCountC = counterStack[top];
                }
            }

            if (ch == 'C') {
                info->hasC = 1;
                if (top >= 0)
                    info->parentC = nodeStack[top];
            }

            if (expression[i + 1] == '(')
                info->nonLeafCount++;
            else
                info->leafCount++;

            if (depth > info->height)
                info->height = depth;

            lastNode = ch;
            state = AFTER_NODE;
        }
        else if (ch == '(') {
            if (state != AFTER_NODE)
                return errorMessage("여는 괄호는 노드 이름 바로 뒤에만 올 수 있습니다.");

            if (top + 1 >= MAX_NODES)
                return errorMessage("스택의 최대 크기를 초과했습니다.");

            top++;
            nodeStack[top] = lastNode;
            counterStack[top] = 0;

            if (lastNode == 'C')
                cLevel = top;

            state = EXPECT_NODE;
        }
        else if (ch == ',') {
            if (top < 0 || state == EXPECT_NODE)
                return errorMessage("쉼표의 위치가 올바르지 않습니다.");

            state = EXPECT_NODE;
        }
        else if (ch == ')') {
            if (top < 0 || state == EXPECT_NODE)
                return errorMessage("닫는 괄호의 위치가 잘못되었거나 자식이 비어 있습니다.");

            /* The completed counter is this parent's degree. */
            if (counterStack[top] > info->degree)
                info->degree = counterStack[top];

            if (top == cLevel)
                cLevel = -1;

            top--;
            state = AFTER_SUBTREE;
        }
        else {
            return errorMessage("영문 대문자, 괄호, 쉼표만 사용할 수 있습니다.");
        }
    }

    if (top != -1 || state == EXPECT_NODE)
        return errorMessage("괄호가 닫히지 않았거나 노드가 빠져 있습니다.");

    /* Labels must be A..N without gaps. Their scan order need not be sorted. */
    for (i = 0; i < info->nodeCount; i++) {
        if (!used[i])
            return errorMessage("노드 이름은 A부터 알파벳을 빠짐없이 사용해야 합니다.");
    }

    info->childrenC[info->childCountC] = '\0';
    return 1;
}

/* A second sequential scan prints the validated input by depth. */
void printTree(const char expression[])
{
    int depth = 0;
    int i;

    for (i = 0; expression[i] != '\0'; i++) {
        char ch = expression[i];

        if (ch == '(') {
            depth++;
        }
        else if (ch == ')') {
            depth--;
        }
        else if (ch >= 'A' && ch <= 'Z') {
            int level;

            for (level = 1; level < depth; level++)
                printf("    ");

            if (depth > 0)
                printf("+---");

            printf("%c\n", ch);
        }
    }
}

void printInfo(const TreeInfo *info)
{
    int i;

    printf("\n전체 노드의 수: %d\n", info->nodeCount);
    printf("단말 노드의 수: %d\n", info->leafCount);
    printf("비단말 노드의 수: %d\n", info->nonLeafCount);
    printf("트리의 높이 (간선 수, 루트 깊이 0): %d\n", info->height);
    printf("레벨 수 (루트 레벨 1): %d\n", info->height + 1);
    printf("트리의 차수: %d\n", info->degree);

    if (!info->hasC) {
        printf("노드 C의 부모 노드: C가 존재하지 않습니다.\n");
        printf("노드 C의 자식 노드: C가 존재하지 않습니다.\n");
    }
    else {
        if (info->parentC != '\0')
            printf("노드 C의 부모 노드: %c\n", info->parentC);
        else
            printf("노드 C의 부모 노드: 없음\n");

        printf("노드 C의 자식 노드: ");
        if (info->childCountC == 0) {
            printf("없음\n");
        }
        else {
            for (i = 0; i < info->childCountC; i++) {
                if (i > 0)
                    printf(", ");
                printf("%c", info->childrenC[i]);
            }
            printf("\n");
        }
    }
}

int main(void)
{
    char expression[MAX_INPUT];
    TreeInfo info;

    printf("트리를 괄호 표기법으로 입력하세요: ");
    fflush(stdout);

    if (!readExpression(expression)) {
        errorMessage("입력이 너무 길거나 읽을 수 없는 문자가 있습니다.");
        return 1;
    }

    if (!analyzeTree(expression, &info))
        return 1;

    printInfo(&info);
    printf("\n트리의 계층적 출력:\n");
    printTree(expression);

    return 0;
}

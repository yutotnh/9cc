#include "9cc.h"

// 現在着目しているトークン
Token *token;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != (size_t)token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != (size_t)token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// EBNF
// program    = stmt*
void program() {
  int i = 0;
  while (!at_eof())
    code[i++] = stmt();
  code[i] = NULL;
}

// EBNF
// stmt       = expr ";"
Node *stmt() {
  Node *node = expr();
  expect(";");
  return node;
}

// EBNF
// expr       = assign
Node *expr() {
  return assign();
}

// ENBF
// assign     = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

// EBNF
// equality   = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

// EBNF
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      // 両辺を入れ替えて、LT(<)として扱う
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      // 両辺を入れ替えて、LE(<=)として扱う
      node = new_node(ND_LE, add(), node);
    else
      return node;
  }
}

// ENBF
// add        = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// EBNF
// mul        = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// EBNF
// unary      = ("+" | "-")? primary
Node *unary() {
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());
  return primary();
}

// EBNF
// primary    = num | ident | "(" expr ")"
Node *primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // 次のトークンが識別子なら、変数
  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->offset = (tok->str[0] - 'a' + 1) * 8;
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

void tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      cur = new_token(TK_IDENT, cur, p++, 1);
      cur->len = 1;
      continue;
    }

    if ((strncmp(p, "==", 2) == 0) || (strncmp(p, "!=", 2) == 0) ||
        (strncmp(p, "<=", 2) == 0) || (strncmp(p, ">=", 2) == 0)) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    switch (*p) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
    case '<':
    case '>':
    case '=':
    case ';':
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
      break;

    default:
      break;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *start_p = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - start_p;
      continue;
    }

    error("トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);

  token = head.next;
}

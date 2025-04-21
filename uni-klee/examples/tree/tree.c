#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct node {
    int data;
    int height;
    struct node *parent;
    struct node *left;
    struct node *right;
};

int max(int a, int b) {
    return (a > b) ? a : b;
}

int height(struct node *node) {
    if (node == NULL) {
        return 0;
    } else {
        return node->height;
    }
}

int balance_factor(struct node *node) {
    if (node == NULL) {
        return 0;
    } else {
        return height(node->left) - height(node->right);
    }
}

struct node* new_node(int data) {
    struct node* node = (struct node*) malloc(sizeof(struct node));
    node->data = data;
    node->height = 1;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    return(node);
}

struct node* right_rotate(struct node *y) {
    struct node *x = y->left;
    struct node *t2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = t2;

    // Update heights
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    // Update parents
    x->parent = y->parent;
    y->parent = x;
    if (t2 != NULL) {
        t2->parent = y;
    }

    return x;
}

struct node* left_rotate(struct node *x) {
    struct node *y = x->right;
    struct node *t2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = t2;

    // Update heights
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    // Update parents
    y->parent = x->parent;
    x->parent = y;
    if (t2 != NULL) {
        t2->parent = x;
    }

    return y;
}

struct node* insert(struct node* node, int data) {
    // Perform normal BST insertion
    if (node == NULL) {
        return(new_node(data));
    }

    if (data < node->data) {
        node->left = insert(node->left, data);
        node->left->parent = node;
    } else if (data > node->data) {
        node->right = insert(node->right, data);
        node->right->parent = node;
    } else {
        return node;
    }

    // Update height of this ancestor node
    node->height = 1 + max(height(node->left), height(node->right));

    // Check if the node became unbalanced
    int balance = balance_factor(node);

    // Left Left Case
    if (balance > 1 && data < node->left->data) {
        return right_rotate(node);
    }

    // Right Right Case
    if (balance <= -1 && data > node->right->data) { // BUG!!!
        return left_rotate(node);
    }

    // Left Right Case
    if (balance > 1 && data < node->left->data) {
      node->left = left_rotate(node->left);
      return right_rotate(node);
    }
    // Right Left Case
    if (balance < -1 && data < node->right->data) {
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }
    // Return the (unchanged) node pointer
    return node;
}

void inorder_traversal(struct node* node) {
  if (node == NULL) {
    return;
  }
  inorder_traversal(node->left);
  printf("%d ", node->data);
  inorder_traversal(node->right);
}

void print_level_order(struct node *root) {
    if (root == NULL) {
        return;
    }

    // Create a queue for level order traversal
    struct node **queue = malloc(sizeof(struct node *) * 1000);
    int front = 0;
    int rear = 0;

    // Enqueue the root node
    queue[rear++] = root;
    printf("\n");

    while (front < rear) {
        int level_size = rear - front;

        // Print the nodes at the current level
        for (int i = 0; i < level_size; i++) {
            struct node *node = queue[front++];
            printf("%d ", node->data);

            // Enqueue the left child, if any
            if (node->left != NULL) {
                queue[rear++] = node->left;
            }

            // Enqueue the right child, if any
            if (node->right != NULL) {
                queue[rear++] = node->right;
            }
        }

        printf("\n");
    }

    free(queue);
}

int get_max_height(struct node *root) {
  if (root == NULL)
    return 0;
  int left_height = get_max_height(root->left);
  int right_height = get_max_height(root->right);
  if (left_height > right_height)
    return left_height + 1;
  else
    return right_height + 1;
}

int calculate_balance_factor(struct node *root) {
  if (root == NULL)
    return 0;
  int left_height = get_max_height(root->left);
  int right_height = get_max_height(root->right);
  assert(abs(left_height - right_height) <= 1);
  return left_height - right_height;
}

int main(int argc, char * argv[]) {
  char *filename = argv[1];
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    return 1;
  }
  struct node* root = NULL;
  int num_read = 0;
  while (fscanf(fp, "%d", &num_read) != EOF) {
    root = insert(root, num_read);
    print_level_order(root);
  }
  print_level_order(root);
  int result = calculate_balance_factor(root);
  printf("\nresult = %d\n", result);
  fclose(fp);
  return 0;
}
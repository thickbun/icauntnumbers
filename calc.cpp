#include <iostream>
#include <string>
#include <cassert>
#include <sstream>
#include <functional>


namespace engine {
    // Типы символов, которые может обработать калькулятор
    enum Token {
        addition,
        subtraction,
        multiplication,
        division,
        opened_parentheses,
        closed_parentheses,
        number,
        eof,
    };

    // Класс бьет строку по токенам
    class Tokenizer {
    private:
        // выражение, которое считает калькулятор
        std::string input;
    public:
        Token current_token = engine::number;
        char current_char = 1;
        double number = 0;

        int position = 0;

        void next_char() {
            char symbol = this->input[this->position];
            this->position++;

            this->current_char = symbol < 0 ? '\0' : symbol;
        }

        /*
         * Input setter
         * Use for request your computational problem
         * */
        void set_input(std::string _input) {
            position = 0;
            current_char = 1;
            current_token = engine::number;
            this->input = std::move(_input);

            next_char();
            next_token();
        }

        void next_token() {
            // пропускаем пробелы
            while (this->current_char == ' ') {
                this->next_char();
            }

            /*
             * Если не пробел, то оперделяем
             * какой из доступных символов
             *
             * */
            switch (this->current_char) {
                case '\0':
                    this->current_token = engine::eof;
                    return;
                case '+':
                    this->next_char();
                    this->current_token = engine::addition;
                    return;
                case '-':
                    this->next_char();
                    this->current_token = engine::subtraction;
                    return;
                case '*':
                    this->next_char();
                    this->current_token = engine::multiplication;
                    return;
                case '/':
                    this->next_char();
                    this->current_token = engine::division;
                    return;
                case '(':
                    this->next_char();
                    this->current_token = engine::opened_parentheses;
                    return;
                case ')':
                    this->next_char();
                    this->current_token = engine::closed_parentheses;
                    return;
            }

            // обрабатываем число
            if (isdigit(this->current_char) || this->current_char == '.') {
                bool is_decimal = false;
                std::stringstream string_builder;

                while (isdigit(this->current_char) || (!is_decimal && this->current_char == '.')) {
                    string_builder << this->current_char;
                    is_decimal = this->current_char == '.';
                    next_char();
                }

                // конвертируем строку в число
                this->number = strtod(string_builder.str().c_str(), nullptr);
                this->current_token = engine::number;
                return;
            }

            // Получили символ, который не поддерживается калькулятором
            std::cout << "Current char : |" << current_char << "|" << std::endl;
            throw std::logic_error(&"Not supported type of operator: " [ current_char]);
        }

        /*
         * Use input setter instead
         *
            explicit Tokenizer(std::string input) {
                this->input = std::move(input);

                next_char();
                next_token();
            }
        */
        Tokenizer() = default;
    };

    class Node {
        public:
        virtual double eval() = 0;
    };

    // Нода для числа
    class NumberNode : public Node {
    public:
        double number;

        explicit NumberNode(double number) {
            this->number = number;
        }

        double eval() override {
            return number;
        }
    };

    // Нода для бинарных операций
    class BinaryOperationNode : public Node {
    public:
        Node *left_leaf;
        Node *right_leaf;
        std::function<double(double, double)> operation;

        BinaryOperationNode(Node *left_leaf, Node *right_leaf, std::function<double(double, double)> operation) {
            this->left_leaf = left_leaf;
            this->right_leaf = right_leaf;
            this->operation = std::move(operation);
        }

        double eval() override {
            auto left_leaf_value = left_leaf->eval();
            auto right_leaf_value = right_leaf->eval();

            return operation(left_leaf_value, right_leaf_value);
        }
    };

    // Нода для унарных операций
    class UnaryOperationNode : public Node {
    public:
        Node *right_leaf;
        std::function<double(double)> operation;

        UnaryOperationNode(Node *right_leaf, std::function<double(double)> operation) {
            this->right_leaf=right_leaf;
            this->operation = std::move(operation);
        }

        double eval() override {
            auto right_leaf_value = right_leaf->eval();

            return operation(right_leaf_value);
        }
    };

    class Parser {
    public:
        double answer = 0;
        Tokenizer *tokenizer;

        explicit Parser(Tokenizer *tokenizer) {
            this->tokenizer = tokenizer;
        }

        void clear() {
            this->tokenizer->position = 0;
            this->tokenizer->current_char = 1;
            this->tokenizer->current_token = engine::eof;
        }

        // Обрабатываем строку до конца
        Node* parse_expression() {
            Node *expression = parse_addition_and_subtraction_operators();

            if (tokenizer->current_token != engine::eof)
                throw std::logic_error("Not understandable expression");

            clear();

            this->answer = expression->eval();
            return expression;
        }

        // Обрабатываем операции сложения и вычитания
        Node* parse_addition_and_subtraction_operators() {
            auto left_leaf = parse_multiplication_and_division_operators();

            while (true) {
                std::function<double(double, double)> operation = nullptr;
                if (tokenizer->current_token == engine::addition) {
                    operation = [](double a, double b) -> double { return a + b; };
                }
                else if (tokenizer->current_token == engine::subtraction) {
                    operation = [](double a, double b) -> double { return a - b; };
                }

                if (operation == nullptr)
                    return left_leaf;
                tokenizer->next_token();

                auto right_leaf = parse_multiplication_and_division_operators();

                left_leaf = new BinaryOperationNode(left_leaf, right_leaf, operation);
            }
        }

        Node* parse_multiplication_and_division_operators() {
            auto left_leaf = parse_unary_operator();

            while (true) {
                std::function<double(double, double)> operation = nullptr;
                if (tokenizer->current_token == engine::multiplication) {
                    operation = [](double a, double b) -> double { return a * b; };
                }
                else if (tokenizer->current_token == engine::division) {
                    operation = [](double a, double b) -> double { return a / b; };
                }

                if (operation == nullptr)
                    return left_leaf;
                tokenizer->next_token();

                auto right_leaf = parse_unary_operator();

                left_leaf = new BinaryOperationNode(left_leaf, right_leaf, operation);
            }
        }

        Node* parse_unary_operator() {
            while (true) {
                if (tokenizer->current_token == engine::addition) {
                    tokenizer->next_token();
                    continue;
                }

                if (tokenizer->current_token == engine::subtraction) {
                    tokenizer->next_token();

                    auto right = parse_unary_operator();
                    return new UnaryOperationNode(right, [](double a) -> double { return -a; });
                }
                return parse_leaf();
            }
        }

        Node* parse_leaf() {
            if (tokenizer->current_token == engine::number) {
                auto *node = new NumberNode(tokenizer->number);
                tokenizer->next_token();
                return node;
            }

            if (tokenizer->current_token == engine::opened_parentheses) {
                tokenizer->next_token();

                auto node = parse_addition_and_subtraction_operators();

                if (tokenizer->current_token != engine::closed_parentheses)
                    throw std::logic_error("Missing parentheses");
                tokenizer->next_token();

                return node;
            }

            throw std::logic_error(&"Unexpect token: " [ tokenizer->current_token]);
        }
    };
}



int main(int argc, char *argv[]) {
    std::string str;
    if (argc == 1) {
        std::getline(std::cin, str);
    } else {
        str = std::string(argv[1]);
    }

    auto parser = new engine::Parser(new engine::Tokenizer);

    parser->tokenizer->set_input(str);
    parser->parse_expression();

    std::cout << parser->answer << std::endl;
    return 0;
}
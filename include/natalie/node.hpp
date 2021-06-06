#pragma once

#include "natalie/gc.hpp"
#include "natalie/managed_vector.hpp"
#include "natalie/token.hpp"

namespace Natalie {

using namespace TM;

class Node : public Cell {
public:
    NAT_MAKE_NONCOPYABLE(Node);

    enum class Type {
        Alias,
        Arg,
        Array,
        Assignment,
        AttrAssign,
        Begin,
        BeginRescue,
        Block,
        BlockPass,
        Break,
        Call,
        Case,
        CaseWhen,
        Class,
        Colon2,
        Colon3,
        Constant,
        Def,
        Defined,
        EvaluateToString,
        False,
        Hash,
        Identifier,
        If,
        Iter,
        InterpolatedRegexp,
        InterpolatedShell,
        InterpolatedString,
        KeywordArg,
        Literal,
        LogicalAnd,
        LogicalOr,
        Match,
        Module,
        MultipleAssignment,
        Next,
        Nil,
        NilSexp,
        Not,
        OpAssign,
        OpAssignAccessor,
        OpAssignAnd,
        OpAssignOr,
        Range,
        Regexp,
        Return,
        SafeCall,
        Sclass,
        Self,
        Splat,
        SplatAssignment,
        StabbyProc,
        String,
        Super,
        Symbol,
        True,
        Until,
        While,
        Yield,
    };

    Node(Token *token)
        : m_token { token } { }

    virtual ValuePtr to_ruby(Env *env) {
        NAT_UNREACHABLE();
    }

    virtual Type type() { NAT_UNREACHABLE(); }

    virtual bool is_callable() { return false; }

    const String *file() { return m_token->file(); }
    size_t line() { return m_token->line(); }
    size_t column() { return m_token->column(); }

    Token *token() { return m_token; }

    virtual void visit_children(Visitor &visitor) override {
        visitor.visit(m_token);
    }

protected:
    Token *m_token { nullptr };
};

class NodeWithArgs : public Node {
public:
    NodeWithArgs(Token *token)
        : Node { token } { }

    void add_arg(Node *arg) {
        m_args.push(arg);
    }

    Vector<Node *> *args() { return &m_args; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        for (auto node : m_args) {
            visitor.visit(node);
        }
    }

protected:
    Vector<Node *> m_args {};
};

class SymbolNode;

class AliasNode : public Node {
public:
    AliasNode(Token *token, SymbolNode *new_name, SymbolNode *existing_name)
        : Node { token }
        , m_new_name { new_name }
        , m_existing_name { existing_name } { }

    virtual Type type() override { return Type::Alias; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override;

private:
    SymbolNode *m_new_name { nullptr };
    SymbolNode *m_existing_name { nullptr };
};

class ArgNode : public Node {
public:
    ArgNode(Token *token)
        : Node { token } { }

    ArgNode(Token *token, const char *name)
        : Node { token }
        , m_name { new (AllocationStrategy::DelayCollection) String(name) } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Arg; }

    virtual ValuePtr to_ruby(Env *) override;

    const char *name() { return m_name->c_str(); }

    bool splat() { return m_splat; }
    void set_splat(bool splat) { m_splat = splat; }

    bool block_arg() { return m_block_arg; }
    void set_block_arg(bool block_arg) { m_block_arg = block_arg; }

    Node *value() { return m_value; }
    void set_value(Node *value) { m_value = value; }

    void add_to_locals(Env *env, ManagedVector<SymbolValue *> *locals) {
        locals->push(SymbolValue::intern(m_name));
    }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_name);
        visitor.visit(m_value);
    }

protected:
    const String *m_name { nullptr };
    bool m_block_arg { false };
    bool m_splat { false };
    Node *m_value { nullptr };
};

class ArrayNode : public Node {
public:
    ArrayNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Array; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    Vector<Node *> *nodes() { return &m_nodes; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        for (auto node : m_nodes) {
            visitor.visit(node);
        }
    }

protected:
    Vector<Node *> m_nodes {};
};

class BlockPassNode : public Node {
public:
    BlockPassNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::BlockPass; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_node);
    }

protected:
    Node *m_node { nullptr };
};

class BreakNode : public NodeWithArgs {
public:
    BreakNode(Token *token, Node *arg = nullptr)
        : NodeWithArgs { token }
        , m_arg { arg } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Break; }

    virtual void visit_children(Visitor &visitor) override {
        NodeWithArgs::visit_children(visitor);
        visitor.visit(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};

class IdentifierNode;

class AssignmentNode : public Node {
public:
    AssignmentNode(Token *token, Node *identifier, Node *value)
        : Node { token }
        , m_identifier { identifier }
        , m_value { value } {
        assert(m_identifier);
        assert(m_value);
    }

    virtual Type type() override { return Type::Assignment; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_identifier);
        visitor.visit(m_value);
    }

protected:
    Node *m_identifier { nullptr };
    Node *m_value { nullptr };
};

class BlockNode;
class BeginRescueNode;

class BeginNode : public Node {
public:
    BeginNode(Token *token, BlockNode *body)
        : Node { token }
        , m_body { body } {
        assert(m_body);
    }

    virtual Type type() override { return Type::Begin; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_rescue_node(BeginRescueNode *node) { m_rescue_nodes.push(node); }
    bool no_rescue_nodes() { return m_rescue_nodes.size() == 0; }

    bool has_ensure_body() { return m_ensure_body ? true : false; }
    void set_else_body(BlockNode *else_body) { m_else_body = else_body; }
    void set_ensure_body(BlockNode *ensure_body) { m_ensure_body = ensure_body; }

    BlockNode *body() { return m_body; }

    virtual void visit_children(Visitor &visitor) override;

protected:
    BlockNode *m_body { nullptr };
    BlockNode *m_else_body { nullptr };
    BlockNode *m_ensure_body { nullptr };
    Vector<BeginRescueNode *> m_rescue_nodes {};
};

class BeginRescueNode : public Node {
public:
    BeginRescueNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::BeginRescue; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_exception_node(Node *node) {
        m_exceptions.push(node);
    }

    void set_exception_name(IdentifierNode *name) {
        m_name = name;
    }

    void set_body(BlockNode *body) { m_body = body; }

    Node *name_to_node();

    virtual void visit_children(Visitor &visitor) override;

protected:
    IdentifierNode *m_name { nullptr };
    Vector<Node *> m_exceptions {};
    BlockNode *m_body { nullptr };
};

class BlockNode : public Node {
public:
    BlockNode(Token *token)
        : Node { token } { }

    BlockNode(Token *token, Node *single_node)
        : Node { token } {
        add_node(single_node);
    }

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual Type type() override { return Type::Block; }

    virtual ValuePtr to_ruby(Env *) override;
    ValuePtr to_ruby_with_name(Env *, const char *);

    Vector<Node *> *nodes() { return &m_nodes; }
    bool is_empty() { return m_nodes.is_empty(); }

    bool has_one_node() { return m_nodes.size() == 1; }

    Node *without_unnecessary_nesting() {
        if (has_one_node())
            return m_nodes[0];
        else
            return this;
    }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        for (auto node : m_nodes) {
            visitor.visit(node);
        }
    }

protected:
    Vector<Node *> m_nodes {};
};

class CallNode : public NodeWithArgs {
public:
    CallNode(Token *token, Node *receiver, const char *message)
        : NodeWithArgs { token }
        , m_receiver { receiver }
        , m_message { new (AllocationStrategy::DelayCollection) String(message) } {
        assert(m_receiver);
        assert(m_message);
    }

    CallNode(Token *token, CallNode &node)
        : NodeWithArgs { token }
        , m_receiver { node.m_receiver }
        , m_message { node.m_message } {
        for (auto arg : node.m_args) {
            add_arg(arg);
        }
    }

    virtual Type type() override { return Type::Call; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual bool is_callable() override { return true; }

    Node *receiver() { return m_receiver; }

    const char *message() { return m_message->c_str(); }

    void set_message(const String *message) {
        assert(message);
        m_message = message;
    }

    void set_message(const char *message) {
        assert(message);
        m_message = new String(message);
    }

    virtual void visit_children(Visitor &visitor) override {
        NodeWithArgs::visit_children(visitor);
        visitor.visit(m_receiver);
        visitor.visit(m_message);
    }

protected:
    Node *m_receiver { nullptr };
    const String *m_message { nullptr };
};

class CaseNode : public Node {
public:
    CaseNode(Token *token, Node *subject)
        : Node { token }
        , m_subject { subject } {
        assert(m_subject);
    }

    virtual Type type() override { return Type::Case; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_when_node(Node *node) {
        m_when_nodes.push(node);
    }

    void set_else_node(BlockNode *node) {
        m_else_node = node;
    }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_subject);
        visitor.visit(m_else_node);
        for (auto node : m_when_nodes) {
            visitor.visit(node);
        }
    }

protected:
    Node *m_subject { nullptr };
    Vector<Node *> m_when_nodes {};
    BlockNode *m_else_node { nullptr };
};

class CaseWhenNode : public Node {
public:
    CaseWhenNode(Token *token, Node *condition, BlockNode *body)
        : Node { token }
        , m_condition { condition }
        , m_body { body } {
        assert(m_condition);
        assert(m_body);
    }

    virtual Type type() override { return Type::CaseWhen; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_condition);
        visitor.visit(m_body);
    }

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
};

class AttrAssignNode : public CallNode {
public:
    AttrAssignNode(Token *token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    AttrAssignNode(Token *token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::AttrAssign; }

    virtual ValuePtr to_ruby(Env *) override;
};

class SafeCallNode : public CallNode {
public:
    SafeCallNode(Token *token, Node *receiver, const char *message)
        : CallNode { token, receiver, message } { }

    SafeCallNode(Token *token, CallNode &node)
        : CallNode { token, node } { }

    virtual Type type() override { return Type::SafeCall; }

    virtual ValuePtr to_ruby(Env *) override;
};

class ConstantNode;

class ClassNode : public Node {
public:
    ClassNode(Token *token, ConstantNode *name, Node *superclass, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_superclass { superclass }
        , m_body { body } { }

    virtual Type type() override { return Type::Class; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override;

protected:
    ConstantNode *m_name { nullptr };
    Node *m_superclass { nullptr };
    BlockNode *m_body { nullptr };
};

class Colon2Node : public Node {
public:
    Colon2Node(Token *token, Node *left, const char *name)
        : Node { token }
        , m_left { left }
        , m_name { new (AllocationStrategy::DelayCollection) String(name) } {
        assert(m_left);
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon2; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_left);
        visitor.visit(m_name);
    }

protected:
    Node *m_left { nullptr };
    const String *m_name { nullptr };
};

class Colon3Node : public Node {
public:
    Colon3Node(Token *token, const char *name)
        : Node { token }
        , m_name { new (AllocationStrategy::DelayCollection) String(name) } {
        assert(m_name);
    }

    virtual Type type() override { return Type::Colon3; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_name);
    }

protected:
    const String *m_name { nullptr };
};

class ConstantNode : public Node {
public:
    ConstantNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Constant; }

    virtual ValuePtr to_ruby(Env *) override;

    const char *name() { return m_token->literal(); }
};

class LiteralNode : public Node {
public:
    LiteralNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Literal; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }
    Value::Type value_type() { return m_value->type(); }

    virtual void visit_children(Visitor &visitor) override;

protected:
    ValuePtr m_value { nullptr };
};

class DefinedNode : public Node {
public:
    DefinedNode(Token *token, Node *arg)
        : Node { token }
        , m_arg { arg } {
        assert(arg);
    }

    virtual Type type() override { return Type::Defined; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};

class DefNode : public Node {
public:
    DefNode(Token *token, Node *self_node, IdentifierNode *name, ManagedVector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_self_node { self_node }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    DefNode(Token *token, IdentifierNode *name, ManagedVector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_args { args }
        , m_body { body } { }

    virtual Type type() override { return Type::Def; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override;

protected:
    SexpValue *build_args_sexp(Env *);

    Node *m_self_node { nullptr };
    IdentifierNode *m_name { nullptr };
    ManagedVector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

class EvaluateToStringNode : public Node {
public:
    EvaluateToStringNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::EvaluateToString; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_node);
    }

protected:
    Node *m_node { nullptr };
};

class FalseNode : public Node {
public:
    FalseNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::False; }
};

class HashNode : public Node {
public:
    HashNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Array; }

    virtual ValuePtr to_ruby(Env *) override;

    void add_node(Node *node) {
        m_nodes.push(node);
    }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        for (auto node : m_nodes) {
            visitor.visit(node);
        }
    }

protected:
    Vector<Node *> m_nodes {};
};

class IdentifierNode : public Node {
public:
    IdentifierNode(Token *token, bool is_lvar)
        : Node { token }
        , m_is_lvar { is_lvar } { }

    virtual Type type() override { return Type::Identifier; }

    virtual ValuePtr to_ruby(Env *) override;

    Token::Type token_type() { return m_token->type(); }

    const char *name() { return m_token->literal(); }

    void append_to_name(char c) {
        auto literal = new String(m_token->literal());
        literal->append_char(c);
        m_token->set_literal(literal);
    }

    virtual bool is_callable() override {
        switch (token_type()) {
        case Token::Type::BareName:
        case Token::Type::Constant:
            return !m_is_lvar;
        default:
            return false;
        }
    }

    bool is_lvar() { return m_is_lvar; }
    void set_is_lvar(bool is_lvar) { m_is_lvar = is_lvar; }

    nat_int_t nth_ref() {
        auto str = name();
        size_t len = strlen(str);
        if (strcmp(str, "$0") == 0)
            return 0;
        if (len <= 1)
            return 0;
        int ref = 0;
        for (size_t i = 1; i < len; i++) {
            char c = str[i];
            if (i == 1 && c == '0')
                return 0;
            int num = c - 48;
            if (num < 0 || num > 9)
                return 0;
            ref *= 10;
            ref += num;
        }
        return ref;
    }

    SexpValue *to_assignment_sexp(Env *);

    SymbolValue *assignment_type(Env *env) {
        switch (token_type()) {
        case Token::Type::BareName:
            return SymbolValue::intern("lasgn");
        case Token::Type::ClassVariable:
            return SymbolValue::intern("cvdecl");
        case Token::Type::Constant:
            return SymbolValue::intern("cdecl");
        case Token::Type::GlobalVariable:
            return SymbolValue::intern("gasgn");
        case Token::Type::InstanceVariable:
            return SymbolValue::intern("iasgn");
        default:
            NAT_UNREACHABLE();
        }
    }

    SymbolValue *to_symbol(Env *env) {
        return SymbolValue::intern(name());
    }

    void add_to_locals(Env *env, ManagedVector<SymbolValue *> *locals) {
        if (token_type() == Token::Type::BareName)
            locals->push(to_symbol(env));
    }

protected:
    bool m_is_lvar { false };
};

class IfNode : public Node {
public:
    IfNode(Token *token, Node *condition, Node *true_expr, Node *false_expr)
        : Node { token }
        , m_condition { condition }
        , m_true_expr { true_expr }
        , m_false_expr { false_expr } {
        assert(m_condition);
        assert(m_true_expr);
        assert(m_false_expr);
    }

    virtual Type type() override { return Type::If; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_condition);
        visitor.visit(m_true_expr);
        visitor.visit(m_false_expr);
    }

protected:
    Node *m_condition { nullptr };
    Node *m_true_expr { nullptr };
    Node *m_false_expr { nullptr };
};

class IterNode : public Node {
public:
    IterNode(Token *token, Node *call, ManagedVector<Node *> *args, BlockNode *body)
        : Node { token }
        , m_call { call }
        , m_args { args }
        , m_body { body } {
        assert(m_call);
        assert(m_args);
        assert(m_body);
    }

    virtual Type type() override { return Type::Iter; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_call);
        visitor.visit(m_body);
        visitor.visit(m_args);
    }

protected:
    SexpValue *build_args_sexp(Env *);

    Node *m_call { nullptr };
    ManagedVector<Node *> *m_args { nullptr };
    BlockNode *m_body { nullptr };
};

class InterpolatedNode : public Node {
public:
    InterpolatedNode(Token *token)
        : Node { token } { }

    void add_node(Node *node) { m_nodes.push(node); };

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        for (auto node : m_nodes) {
            visitor.visit(node);
        }
    }

protected:
    Vector<Node *> m_nodes {};
};

class InterpolatedRegexpNode : public InterpolatedNode {
public:
    InterpolatedRegexpNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedRegexp; }

    virtual ValuePtr to_ruby(Env *) override;
};

class InterpolatedShellNode : public InterpolatedNode {
public:
    InterpolatedShellNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedShell; }

    virtual ValuePtr to_ruby(Env *) override;
};

class InterpolatedStringNode : public InterpolatedNode {
public:
    InterpolatedStringNode(Token *token)
        : InterpolatedNode { token } { }

    virtual Type type() override { return Type::InterpolatedString; }

    virtual ValuePtr to_ruby(Env *) override;
};

class KeywordArgNode : public ArgNode {
public:
    KeywordArgNode(Token *token, const char *name)
        : ArgNode { token, name } { }

    virtual Type type() override { return Type::KeywordArg; }

    virtual ValuePtr to_ruby(Env *) override;
};

class LogicalAndNode : public Node {
public:
    LogicalAndNode(Token *token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalAnd; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *left() { return m_left; }
    Node *right() { return m_right; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_left);
        visitor.visit(m_right);
    }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

class LogicalOrNode : public Node {
public:
    LogicalOrNode(Token *token, Node *left, Node *right)
        : Node { token }
        , m_left { left }
        , m_right { right } {
        assert(m_left);
        assert(m_right);
    }

    virtual Type type() override { return Type::LogicalOr; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *left() { return m_left; }
    Node *right() { return m_right; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_left);
        visitor.visit(m_right);
    }

protected:
    Node *m_left { nullptr };
    Node *m_right { nullptr };
};

class RegexpNode;

class MatchNode : public Node {
public:
    MatchNode(Token *token, RegexpNode *regexp, Node *arg, bool regexp_on_left)
        : Node { token }
        , m_regexp { regexp }
        , m_arg { arg }
        , m_regexp_on_left { regexp_on_left } { }

    virtual Type type() override { return Type::Match; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override;

protected:
    RegexpNode *m_regexp { nullptr };
    Node *m_arg { nullptr };
    bool m_regexp_on_left { false };
};

class ModuleNode : public Node {
public:
    ModuleNode(Token *token, ConstantNode *name, BlockNode *body)
        : Node { token }
        , m_name { name }
        , m_body { body } { }

    virtual Type type() override { return Type::Module; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_name);
        visitor.visit(m_body);
    }

protected:
    ConstantNode *m_name { nullptr };
    BlockNode *m_body { nullptr };
};

class MultipleAssignmentNode : public ArrayNode {
public:
    MultipleAssignmentNode(Token *token)
        : ArrayNode { token } { }

    virtual Type type() override { return Type::MultipleAssignment; }

    virtual ValuePtr to_ruby(Env *) override;
    ArrayValue *to_ruby_with_array(Env *);

    void add_locals(Env *, ManagedVector<SymbolValue *> *);
};

class NextNode : public Node {
public:
    NextNode(Token *token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Next; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};

class NilNode : public Node {
public:
    NilNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Nil; }
};

class NotNode : public Node {
public:
    NotNode(Token *token, Node *expression)
        : Node { token }
        , m_expression { expression } {
        assert(m_expression);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Not; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_expression);
    }

protected:
    Node *m_expression { nullptr };
};

class NilSexpNode : public Node {
public:
    NilSexpNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::NilSexp; }
};

class OpAssignNode : public Node {
public:
    OpAssignNode(Token *token, IdentifierNode *name, Node *value)
        : Node { token }
        , m_name { name }
        , m_value { value } {
        assert(m_name);
        assert(m_value);
    }

    OpAssignNode(Token *token, const String *op, IdentifierNode *name, Node *value)
        : Node { token }
        , m_op { op }
        , m_name { name }
        , m_value { value } {
        assert(m_op);
        assert(m_name);
        assert(m_value);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssign; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_op);
        visitor.visit(m_name);
        visitor.visit(m_value);
    }

protected:
    const String *m_op { nullptr };
    IdentifierNode *m_name { nullptr };
    Node *m_value { nullptr };
};

class OpAssignAccessorNode : public NodeWithArgs {
public:
    OpAssignAccessorNode(Token *token, const String *op, Node *receiver, const String *message, Node *value)
        : NodeWithArgs { token }
        , m_op { op }
        , m_receiver { receiver }
        , m_message { message }
        , m_value { value } {
        assert(m_op);
        assert(m_receiver);
        assert(m_message);
        assert(m_value);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignAccessor; }

    virtual void visit_children(Visitor &visitor) override {
        NodeWithArgs::visit_children(visitor);
        visitor.visit(m_op);
        visitor.visit(m_receiver);
        visitor.visit(m_message);
        visitor.visit(m_value);
    }

protected:
    const String *m_op { nullptr };
    Node *m_receiver { nullptr };
    const String *m_message { nullptr };
    Node *m_value { nullptr };
};

class OpAssignAndNode : public OpAssignNode {
public:
    OpAssignAndNode(Token *token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignAnd; }
};

class OpAssignOrNode : public OpAssignNode {
public:
    OpAssignOrNode(Token *token, IdentifierNode *name, Node *value)
        : OpAssignNode { token, name, value } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::OpAssignOr; }
};

class RangeNode : public Node {
public:
    RangeNode(Token *token, Node *first, Node *last, bool exclude_end)
        : Node { token }
        , m_first { first }
        , m_last { last }
        , m_exclude_end { exclude_end } {
        assert(m_first);
        assert(m_last);
    }

    virtual Type type() override { return Type::Range; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_first);
        visitor.visit(m_last);
    }

protected:
    Node *m_first { nullptr };
    Node *m_last { nullptr };
    bool m_exclude_end { false };
};

class RegexpNode : public Node {
public:
    RegexpNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Regexp; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_value);
    }

protected:
    ValuePtr m_value { nullptr };
};

class ReturnNode : public Node {
public:
    ReturnNode(Token *token, Node *arg = nullptr)
        : Node { token }
        , m_arg { arg } { }

    virtual Type type() override { return Type::Return; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_arg);
    }

protected:
    Node *m_arg { nullptr };
};

class SclassNode : public Node {
public:
    SclassNode(Token *token, Node *klass, BlockNode *body)
        : Node { token }
        , m_klass { klass }
        , m_body { body } { }

    virtual Type type() override { return Type::Sclass; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_klass);
        visitor.visit(m_body);
    }

protected:
    Node *m_klass { nullptr };
    BlockNode *m_body { nullptr };
};

class SelfNode : public Node {
public:
    SelfNode(Token *token)
        : Node { token } { }

    virtual Type type() override { return Type::Self; }

    virtual ValuePtr to_ruby(Env *) override;
};

class ShellNode : public Node {
public:
    ShellNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::String; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_value);
    }

protected:
    ValuePtr m_value { nullptr };
};

class SplatAssignmentNode : public Node {
public:
    SplatAssignmentNode(Token *token)
        : Node { token } { }

    SplatAssignmentNode(Token *token, IdentifierNode *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::SplatAssignment; }

    virtual ValuePtr to_ruby(Env *) override;

    IdentifierNode *node() { return m_node; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_node);
    }

protected:
    IdentifierNode *m_node { nullptr };
};

class SplatNode : public Node {
public:
    SplatNode(Token *token)
        : Node { token } { }

    SplatNode(Token *token, Node *node)
        : Node { token }
        , m_node { node } {
        assert(m_node);
    }

    virtual Type type() override { return Type::Splat; }

    virtual ValuePtr to_ruby(Env *) override;

    Node *node() { return m_node; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_node);
    }

protected:
    Node *m_node { nullptr };
};

class StabbyProcNode : public Node {
public:
    StabbyProcNode(Token *token, ManagedVector<Node *> *args)
        : Node { token }
        , m_args { args } {
        assert(m_args);
    }

    virtual Type type() override { return Type::StabbyProc; }

    virtual ValuePtr to_ruby(Env *) override;

    ManagedVector<Node *> *args() { return m_args; };

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_args);
    }

protected:
    ManagedVector<Node *> *m_args { nullptr };
};

class StringNode : public Node {
public:
    StringNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::String; }

    virtual ValuePtr to_ruby(Env *) override;

    ValuePtr value() { return m_value; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_value);
    }

protected:
    ValuePtr m_value { nullptr };
};

class SymbolNode : public Node {
public:
    SymbolNode(Token *token, ValuePtr value)
        : Node { token }
        , m_value { value } {
        assert(m_value);
    }

    virtual Type type() override { return Type::Symbol; }

    virtual ValuePtr to_ruby(Env *) override;

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_value);
    }

protected:
    ValuePtr m_value { nullptr };
};

class TrueNode : public Node {
public:
    TrueNode(Token *token)
        : Node { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::True; }
};

class SuperNode : public NodeWithArgs {
public:
    SuperNode(Token *token)
        : NodeWithArgs { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Super; }

    bool parens() { return m_parens; }
    void set_parens(bool parens) { m_parens = parens; }

    bool empty_parens() { return m_parens && m_args.is_empty(); }

protected:
    bool m_parens { false };
};

class WhileNode : public Node {
public:
    WhileNode(Token *token, Node *condition, BlockNode *body, bool pre)
        : Node { token }
        , m_condition { condition }
        , m_body { body }
        , m_pre { pre } {
        assert(m_condition);
        assert(m_body);
    }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::While; }

    virtual void visit_children(Visitor &visitor) override {
        Node::visit_children(visitor);
        visitor.visit(m_condition);
        visitor.visit(m_body);
    }

protected:
    Node *m_condition { nullptr };
    BlockNode *m_body { nullptr };
    bool m_pre { false };
};

class UntilNode : public WhileNode {
public:
    UntilNode(Token *token, Node *condition, BlockNode *body, bool pre)
        : WhileNode { token, condition, body, pre } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Until; }
};

class YieldNode : public NodeWithArgs {
public:
    YieldNode(Token *token)
        : NodeWithArgs { token } { }

    virtual ValuePtr to_ruby(Env *) override;

    virtual Type type() override { return Type::Yield; }
};

}

#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace any2ns2 {
namespace fs = std::filesystem;
struct Token { std::string text; size_t line; };
struct ModelMember { std::string name, type, kind, initializer; };
struct ModelType { std::string name, kind, file, parent; std::vector<std::string> bases; std::vector<ModelMember> members; size_t line = 1; size_t open = 0, close = 0; };
struct ModelInstance { std::string type, file, form; size_t line = 1; };
struct ModelReport { std::vector<ModelType> types; std::vector<ModelInstance> instances; size_t csharpFiles = 0; };

inline std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> out; size_t i=0, line=1;
    auto emit=[&](std::string x,size_t l){out.push_back({std::move(x),l});};
    while(i<s.size()) {
        unsigned char c=s[i];
        if(c=='\n'){++line;++i;continue;} if(std::isspace(c)){++i;continue;}
        if(c=='/'&&i+1<s.size()&&s[i+1]=='/'){i+=2;while(i<s.size()&&s[i]!='\n')++i;continue;}
        if(c=='/'&&i+1<s.size()&&s[i+1]=='*'){i+=2;while(i+1<s.size()&&!(s[i]=='*'&&s[i+1]=='/')){if(s[i++]=='\n')++line;}i=std::min(i+2,s.size());continue;}
        // C# interpolated/verbatim/raw strings are intentionally opaque to syntax discovery.
        size_t prefix=i; while(i<s.size()&&(s[i]=='$'||s[i]=='@'))++i;
        if(i<s.size()&&(s[i]=='\"'||s[i]=='\'')) {
            char q=s[i]; bool verbatim=s.substr(prefix,i-prefix).find('@')!=std::string::npos;
            if(q=='\"'&&i+2<s.size()&&s[i+1]=='\"'&&s[i+2]=='\"') {i+=3;while(i+2<s.size()&&s.substr(i,3)!="\"\"\""){if(s[i++]=='\n')++line;}i=std::min(i+3,s.size());}
            else {++i;while(i<s.size()){if(s[i]=='\n')++line;if(s[i]==q){if(verbatim&&i+1<s.size()&&s[i+1]==q){i+=2;continue;}++i;break;}if(!verbatim&&s[i]=='\\'&&i+1<s.size()){i+=2;continue;}++i;}}
            emit(q=='\"'?"<string>":"<char>",line); continue;
        }
        i=prefix; size_t l=line;
        if(std::isalpha(c)||c=='_') {size_t b=i++;while(i<s.size()&&(std::isalnum(static_cast<unsigned char>(s[i]))||s[i]=='_'))++i;emit(s.substr(b,i-b),l);continue;}
        if(std::isdigit(c)){size_t b=i++;while(i<s.size()&&(std::isalnum(static_cast<unsigned char>(s[i]))||s[i]=='.'||s[i]=='_'))++i;emit(s.substr(b,i-b),l);continue;}
        if(i+1<s.size()&&((s[i]=='='&&s[i+1]=='>')||(s[i]==':'&&s[i+1]==':')||(s[i]=='?'&&(s[i+1]=='.'||s[i+1]=='?'))||(s[i]=='='&&s[i+1]=='='))){emit(s.substr(i,2),l);i+=2;continue;}
        emit(std::string(1,s[i++]),l);
    }
    return out;
}
inline std::string joinTokens(const std::vector<Token>& t,size_t b,size_t e) { std::string s; for(size_t i=b;i<e;++i){if(!s.empty()&&t[i].text!="."&&t[i].text!="::"&&t[i].text!=","&&t[i].text!=">"&&t[i].text!="]"&&t[i].text!="("&&t[i].text!="["&&s.back()!='<'&&s.back()!='['&&s.back()!='('&&s.back()!='.')s+=' ';s+=t[i].text;}return s; }
inline bool identifier(const std::string& s){return !s.empty()&&(std::isalpha(static_cast<unsigned char>(s[0]))||s[0]=='_');}
inline std::string json(const std::string& s){std::string o="\"";for(unsigned char c:s){switch(c){case '\"':o+="\\\"";break;case '\\':o+="\\\\";break;case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;default:if(c<32){const char*h="0123456789abcdef";o+="\\u00";o+=h[c>>4];o+=h[c&15];}else o+=static_cast<char>(c);}}return o+"\"";}
inline std::vector<std::string> splitTop(const std::vector<Token>& t,size_t b,size_t e,const std::string& sep){std::vector<std::string> v;size_t start=b;int a=0,p=0,c=0,br=0;for(size_t i=b;i<e;++i){auto&x=t[i].text;if(x=="<")++a;else if(x==">"&&a)--a;else if(x=="(")++p;else if(x==")"&&p)--p;else if(x=="[")++br;else if(x=="]"&&br)--br;else if(x=="{")++c;else if(x=="}"&&c)--c;if(x==sep&&!a&&!p&&!c&&!br){if(start<i)v.push_back(joinTokens(t,start,i));start=i+1;}}if(start<e)v.push_back(joinTokens(t,start,e));return v;}

inline void analyzeMembers(ModelType& ty,const std::vector<Token>& t,size_t begin,size_t end) {
    int depth=1; size_t start=begin; bool isEnum=ty.kind=="enum";
    for(size_t i=begin;i<end;++i){const auto& x=t[i].text;
        if(depth==1&&((!isEnum&&x==";")||(isEnum&&(x==","||x=="}")))){size_t stop=i; if(stop>start){size_t eq=stop;for(size_t j=start;j<stop;++j)if(t[j].text=="="){eq=j;break;}size_t name=eq;while(name>start&&!identifier(t[name-1].text))--name;if(name>start){--name;bool paren=false;for(size_t j=start;j<eq;++j)if(t[j].text=="(")paren=true;
            if(isEnum){if(identifier(t[name].text))ty.members.push_back({t[name].text,"enum-value","value",""});}
            else if(!paren&&identifier(t[name].text)){size_t typeEnd=name;while(typeEnd>start&& (t[typeEnd-1].text=="]"||t[typeEnd-1].text=="["))--typeEnd;size_t typeStart=start;while(typeStart<typeEnd&&(t[typeStart].text=="public"||t[typeStart].text=="private"||t[typeStart].text=="protected"||t[typeStart].text=="internal"||t[typeStart].text=="static"||t[typeStart].text=="readonly"||t[typeStart].text=="const"||t[typeStart].text=="volatile"||t[typeStart].text=="new"||t[typeStart].text=="required"||t[typeStart].text=="unsafe"||t[typeStart].text=="override"||t[typeStart].text=="virtual"||t[typeStart].text=="public"))++typeStart;
                std::string typ=joinTokens(t,typeStart,typeEnd);if(!typ.empty()&&typ!=t[name].text){std::string init=eq<stop?joinTokens(t,eq+1,stop):"";ty.members.push_back({t[name].text,typ,"field",init});}
            }
        }} start=i+1;}
        if(depth==1&&x=="{"){
            size_t close=i+1;int d=1;for(;close<end&&d;++close){if(t[close].text=="{")++d;else if(t[close].text=="}")--d;}
            bool nestedType=false, hasParen=false;
            for(size_t j=start;j<i;++j){const auto& p=t[j].text;if(p=="class"||p=="struct"||p=="record"||p=="enum")nestedType=true;if(p==")")hasParen=true;}
            // Only accessor keywords at the property's own brace level count; nested
            // class members and method bodies must not leak into the containing type.
            bool accessor=false;int ad=0;
            for(size_t j=i+1;j+1<close;++j){if(t[j].text=="{"){++ad;continue;}if(t[j].text=="}"){--ad;continue;}if(ad==0&&(t[j].text=="get"||t[j].text=="set"||t[j].text=="init")){accessor=true;break;}}
            if(nestedType){i=close?close-1:i;start=close;continue;}
            if(accessor){size_t name=i;while(name>start&&!identifier(t[name-1].text))--name;if(name>start){--name;size_t ts=start;while(ts<name&&(t[ts].text=="public"||t[ts].text=="private"||t[ts].text=="protected"||t[ts].text=="internal"||t[ts].text=="static"||t[ts].text=="virtual"||t[ts].text=="override"||t[ts].text=="required"||t[ts].text=="new"))++ts;auto typ=joinTokens(t,ts,name);size_t stop=close;while(stop<end&&t[stop].text!=";")++stop;std::string init=(close<stop&&t[close].text=="=")?joinTokens(t,close+1,stop):"";if(!typ.empty())ty.members.push_back({t[name].text,typ,"property",init});}i=close?close-1:i;start=close;continue;}
            if(hasParen){i=close?close-1:i;start=close;continue;}
        }
        if(x=="{")++depth;else if(x=="}"){--depth;if(depth==1)start=i+1;}
    }
    if(isEnum&&start<end){size_t stop=end,eq=end;for(size_t j=start;j<end;++j)if(t[j].text=="="){eq=j;break;}size_t n=start;while(n<eq&&!identifier(t[n].text))++n;if(n<eq)ty.members.push_back({t[n].text,"enum-value","value",eq<stop?joinTokens(t,eq+1,stop):""});}
}

inline ModelReport analyzeCSharp(const fs::path& root,const std::vector<fs::path>& files){
    ModelReport r;
    for(const auto& rel:files){if(rel.extension()!=".cs"&&rel.extension()!=".CS")continue;++r.csharpFiles;std::ifstream in(root/rel,std::ios::binary);std::ostringstream ss;ss<<in.rdbuf();auto t=tokenize(ss.str());
        for(size_t i=0;i<t.size();++i){std::string kind=t[i].text;if(kind!="class"&&kind!="struct"&&kind!="record"&&kind!="interface"&&kind!="enum")continue;size_t n=i+1;if(kind=="record"&&n<t.size()&&(t[n].text=="class"||t[n].text=="struct"))kind=t[n++].text;if(n>=t.size()||!identifier(t[n].text))continue;
            ModelType d;d.name=t[n].text;d.kind=kind;d.file=rel.generic_string();d.line=t[i].line;
            size_t brace=n+1;int angle=0,paren=0;for(;brace<t.size();++brace){auto&x=t[brace].text;if(x=="<")++angle;else if(x==">"&&angle)--angle;else if(x=="(")++paren;else if(x==")"&&paren)--paren;else if(x=="{"&&!angle&&!paren)break;else if(x==";"&&!angle&&!paren)break;}
            if(brace>=t.size()||t[brace].text!="{") continue;
            int dep=1; size_t close=brace+1;
            for(;close<t.size()&&dep;++close){if(t[close].text=="{")++dep;else if(t[close].text=="}")--dep;}
            d.open=brace;d.close=close?close-1:brace;
            size_t colon=n+1;while(colon<brace&&t[colon].text!=":")++colon;
            if(colon<brace){size_t e=colon+1;while(e<brace&&t[e].text!="where")++e;for(auto&b:splitTop(t,colon+1,e,","))if(!b.empty())d.bases.push_back(b);}
            r.types.push_back(std::move(d));
        }
        for(size_t i=0;i<t.size();++i)if(t[i].text=="new"){
            ModelInstance m;m.file=rel.generic_string();m.line=t[i].line;size_t j=i+1;
            if(j<t.size()&&t[j].text=="("){m.type="<target-typed>";m.form="target-typed creation";r.instances.push_back(m);continue;}
            size_t b=j;int angle=0;while(j<t.size()){const auto&x=t[j].text;if(x=="<")++angle;else if(x==">"&&angle)--angle;if(!identifier(x)&&x!="."&&x!="::"&&x!="<"&&x!=">"&&x!=","&&x!="?"&&x!="["&&x!="]")break;++j;if(!angle&&j<t.size()&&t[j].text=="(")break;}
            if(j>b){m.type=joinTokens(t,b,j);m.form=(j<t.size()&&t[j].text=="(")?"constructor":"initializer/array";r.instances.push_back(std::move(m));}
        }
    }
    // Resolve nesting and direct class/struct inheritance hierarchy from source spans.
    for(auto&child:r.types){size_t best=static_cast<size_t>(-1),span=static_cast<size_t>(-1);for(const auto&parent:r.types)if(parent.file==child.file&&parent.open<child.open&&parent.close>child.close){size_t d=parent.close-parent.open;if(d<span){span=d;best=&parent-&r.types[0];}}if(best!=static_cast<size_t>(-1))child.parent=r.types[best].name;}
    for(auto&ty:r.types){std::ifstream in(root/ty.file,std::ios::binary);std::ostringstream ss;ss<<in.rdbuf();auto t=tokenize(ss.str());if(ty.open<t.size()&&ty.close<t.size()&&ty.close>ty.open)analyzeMembers(ty,t,ty.open+1,ty.close);}
    return r;
}
// Build a conservative static type graph from declarations. This describes source-level
// references only; it does not assert object ownership or runtime reachability.
struct ModelGraphEdge { std::string from, to, relation, via, source; size_t line = 1; };
inline std::string modelTypeId(const ModelType& ty, const std::vector<ModelType>& all) {
    std::string nested = ty.name;
    std::string parent = ty.parent;
    while (!parent.empty()) {
        nested = parent + "." + nested;
        std::string next;
        for (const auto& p : all) if (p.file == ty.file && p.name == parent && p.open < ty.open && p.close > ty.close) { next = p.parent; break; }
        parent = next;
    }
    return ty.file + "::" + nested;
}
inline std::string simpleTypeName(std::string s) {
    size_t cut = s.find_first_of("<[,?* ");
    if (cut != std::string::npos) s.resize(cut);
    size_t scope = s.rfind('.');
    if (scope != std::string::npos) s.erase(0, scope + 1);
    return s;
}
inline bool containsTypeIdentifier(const std::string& text, const std::string& name) {
    if (name.empty()) return false;
    size_t at = 0;
    while ((at = text.find(name, at)) != std::string::npos) {
        const bool left = at == 0 || !(std::isalnum(static_cast<unsigned char>(text[at-1])) || text[at-1] == '_');
        size_t end = at + name.size();
        const bool right = end == text.size() || !(std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_');
        if (left && right) return true;
        at = end;
    }
    return false;
}
inline std::string modelGraphJson(const ModelReport& r) {
    std::vector<ModelGraphEdge> edges;
    auto resolve = [&](const std::string& ref) -> const ModelType* {
        const std::string name = simpleTypeName(ref);
        const ModelType* found = nullptr;
        for (const auto& candidate : r.types) if (candidate.name == name) {
            if (found) return nullptr; // Ambiguous short names are deliberately unresolved.
            found = &candidate;
        }
        return found;
    };
    for (const auto& ty : r.types) {
        const std::string from = modelTypeId(ty, r.types);
        for (const auto& base : ty.bases) if (const auto* target = resolve(base)) {
            edges.push_back({from, modelTypeId(*target, r.types), "inherits-or-implements", "", ty.file, ty.line});
        }
        for (const auto& member : ty.members) {
            for (const auto& target : r.types) {
                if (target.name == ty.name && target.file == ty.file && target.line == ty.line) continue;
                if (!containsTypeIdentifier(member.type, target.name)) continue;
                const bool collection = member.type.find('<') != std::string::npos && member.type.find(target.name) != std::string::npos;
                edges.push_back({from, modelTypeId(target, r.types), collection ? "collection-element" : "member-reference", member.name, ty.file, ty.line});
            }
        }
    }
    std::ostringstream o;
    o << "{\n  \"format\": \"any2ns2.model-graph\",\n  \"version\": 1,\n  \"scope\": \"Conservative static declaration graph; edges are source references, not proven runtime ownership or reachability.\",\n  \"nodes\": [";
    for (size_t i = 0; i < r.types.size(); ++i) {
        const auto& ty = r.types[i];
        o << (i ? "," : "") << "\n    {\"id\":" << json(modelTypeId(ty, r.types)) << ",\"name\":" << json(ty.name) << ",\"kind\":" << json(ty.kind) << ",\"source\":" << json(ty.file) << ",\"line\":" << ty.line << "}";
    }
    o << "\n  ],\n  \"edges\": [";
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& e = edges[i];
        o << (i ? "," : "") << "\n    {\"from\":" << json(e.from) << ",\"to\":" << json(e.to) << ",\"relation\":" << json(e.relation) << ",\"via\":" << json(e.via) << ",\"source\":" << json(e.source) << ",\"line\":" << e.line << "}";
    }
    o << "\n  ]\n}\n";
    return o.str();
}

inline std::string modelJson(const ModelReport&r){std::ostringstream o;o<<"{\n  \"format\": \"any2ns2.model-inventory\",\n  \"version\": 1,\n  \"scope\": \"Static source inventory; original source and assets are preserved byte-for-byte. This is not a compiled runtime model.\",\n  \"language\": \"C#\",\n  \"filesAnalyzed\": "<<r.csharpFiles<<",\n  \"types\": [";
    for(size_t i=0;i<r.types.size();++i){auto&x=r.types[i];o<<(i?",":"")<<"\n    {\"name\":"<<json(x.name)<<",\"kind\":"<<json(x.kind)<<",\"source\":"<<json(x.file)<<",\"line\":"<<x.line<<",\"parent\":"<<(x.parent.empty()?"null":json(x.parent))<<",\"bases\":[";for(size_t j=0;j<x.bases.size();++j)o<<(j?",":"")<<json(x.bases[j]);o<<"],\"members\":[";for(size_t j=0;j<x.members.size();++j){auto&m=x.members[j];o<<(j?",":"")<<"{\"name\":"<<json(m.name)<<",\"type\":"<<json(m.type)<<",\"kind\":"<<json(m.kind)<<",\"initializer\":"<<json(m.initializer)<<"}";}o<<"]}";}
    o<<"\n  ],\n  \"instantiations\": [";for(size_t i=0;i<r.instances.size();++i){auto&x=r.instances[i];o<<(i?",":"")<<"\n    {\"type\":"<<json(x.type)<<",\"form\":"<<json(x.form)<<",\"source\":"<<json(x.file)<<",\"line\":"<<x.line<<"}";}o<<"\n  ]\n}\n";return o.str();}
} // namespace any2ns2

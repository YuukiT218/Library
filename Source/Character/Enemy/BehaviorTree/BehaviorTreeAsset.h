#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

// ---------------------------------------------------------------------------
// ビヘイビアツリーをエディタで編集・保存するためのデータ表現。
//
// 実行時のツリー（BehaviorTree / NodeBase）はポインタで親子が繋がっていて
// 編集には向かないため、「保存される形」をこちらに分離している。
// エディタはこの構造体だけを触り、実行時ツリーは BehaviorTreeBuilder が
// ここから組み立て直す。
// ---------------------------------------------------------------------------

// 行動の実体をどこから持ってくるか
enum class BehaviorActionKind
{
	None,	// 中間ノード。行動を持たず子ノードを選ぶだけ
	Native,	// C++ で実装された行動クラス
	Lua,	// Lua スクリプトで実装された行動
};

NLOHMANN_JSON_SERIALIZE_ENUM(BehaviorActionKind, {
	{ BehaviorActionKind::None,   "None"   },
	{ BehaviorActionKind::Native, "Native" },
	{ BehaviorActionKind::Lua,    "Lua"    },
	})

// 子ノードの選び方。BehaviorTree<T>::SelectRule と 1:1 で対応する。
// テンプレートの外で名前を扱いたいので、ここに独立した列挙を置いている。
enum class BehaviorSelectRule
{
	Non,				// 末端ノード（子を持たない）
	Priority,			// 優先順位
	Sequence,			// シーケンス
	SequentialLooping,	// シーケンシャルルーピング
	Random,				// ランダム
};

NLOHMANN_JSON_SERIALIZE_ENUM(BehaviorSelectRule, {
	{ BehaviorSelectRule::Non,               "Non"               },
	{ BehaviorSelectRule::Priority,          "Priority"          },
	{ BehaviorSelectRule::Sequence,          "Sequence"          },
	{ BehaviorSelectRule::SequentialLooping, "SequentialLooping" },
	{ BehaviorSelectRule::Random,            "Random"            },
	})

// ノードが再生するアニメーション 1 本ぶんの設定。
//
// 行動によっては連続して複数のアニメーションを再生するため配列で持つ。
// Lua 側からは node.clips[i] として参照できる。
struct BehaviorClip
{
	std::string clip;					// アニメーション名（Model::GetAnimationIndex に渡す）
	bool		loop = false;			// ループ再生するか
	bool		rootMotion = true;		// ルートモーションを適用するか
	float		blendSeconds = 0.1f;	// 直前のアニメーションからのブレンド時間
	float		speedScale = 1.0f;		// 再生速度倍率

	// この区間に入ったら次の行動へキャンセルしてよい、という時間（秒）。
	// 負値なら AnimationConfig の advanceInputStartFrame / advanceInputEndFrame を使う。
	// （連続行動で同じアニメでもキャンセルタイミングを変えたい場合にここで上書きする）
	float		cancelStart = -1.0f;
	float		cancelEnd = -1.0f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BehaviorClip,
	clip, loop, rootMotion, blendSeconds, speedScale, cancelStart, cancelEnd)

// ノードごとに持たせる調整用パラメータ。
// Lua スクリプトから node.params.<name> で読めるので、
// スクリプト本体を書き換えずに数値だけ詰められる。
struct BehaviorParam
{
	enum class Type
	{
		Float,
		Int,
		Bool,
		String,
	};

	std::string name;
	Type		type = Type::Float;

	float		floatValue = 0.0f;
	int			intValue = 0;
	bool		boolValue = false;
	std::string stringValue;
};

NLOHMANN_JSON_SERIALIZE_ENUM(BehaviorParam::Type, {
	{ BehaviorParam::Type::Float,  "Float"  },
	{ BehaviorParam::Type::Int,    "Int"    },
	{ BehaviorParam::Type::Bool,   "Bool"   },
	{ BehaviorParam::Type::String, "String" },
	})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BehaviorParam,
	name, type, floatValue, intValue, boolValue, stringValue)

// ノード 1 つぶんの定義
struct BehaviorNodeAsset
{
	// ノードを一意に識別する ID。
	// 名前は重複しうる（同じ行動を並べたシーケンスなど）ので、
	// 親子関係とエディタの選択状態はすべてこの ID で扱う。
	int					id = 0;
	int					parentId = -1;	// ルートは -1

	std::string			name;			// 表示名。BehaviorData のシーケンス管理キーにも使われる
	int					priority = 0;
	BehaviorSelectRule	selectRule = BehaviorSelectRule::Non;

	std::string			judgment;		// 判定クラスのレジストリ名。空なら常に true

	BehaviorActionKind	actionKind = BehaviorActionKind::None;
	std::string			actionName;		// Native のときのレジストリ名
	std::string			actionScript;	// Lua のときのスクリプトパス

	std::vector<BehaviorClip>	clips;
	std::vector<BehaviorParam>	params;

	// エディタ上の見た目に関する情報（実行には影響しない）
	float				editorX = 0.0f;
	float				editorY = 0.0f;
	std::string			comment;

	const BehaviorParam* FindParam(const std::string& paramName) const
	{
		for (const BehaviorParam& param : params)
		{
			if (param.name == paramName) return &param;
		}
		return nullptr;
	}
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BehaviorNodeAsset,
	id, parentId, name, priority, selectRule,
	judgment, actionKind, actionName, actionScript,
	clips, params, editorX, editorY, comment)

// ツリー 1 本ぶん。ノードはフラットな配列で持ち、親子は parentId で表す。
struct BehaviorTreeAsset
{
	std::string						character = "EnemyBoss";
	std::vector<BehaviorNodeAsset>	nodes;

	// 保存・読み込み。失敗したら false を返し、error に理由を入れる。
	bool Load(const std::string& path, std::string& error);
	bool Save(const std::string& path, std::string& error) const;

	BehaviorNodeAsset* FindNode(int id);
	const BehaviorNodeAsset* FindNode(int id) const;

	// ルートノード（parentId が -1 のもの）。無ければ nullptr
	BehaviorNodeAsset* FindRoot();

	// 指定ノードの子を親のノード配列順で返す
	std::vector<BehaviorNodeAsset*> FindChildren(int parentId);
	std::vector<const BehaviorNodeAsset*> FindChildren(int parentId) const;

	// 未使用の ID を払い出す
	int GenerateId() const;

	// ノードを追加して、その参照を返す
	BehaviorNodeAsset& AddNode(int parentId, const std::string& name);

	// ノードとその子孫をまとめて削除する
	void RemoveNodeRecursive(int id);

	// descendantId が ancestorId の子孫かどうか（循環した接続を弾くのに使う）
	bool IsDescendantOf(int descendantId, int ancestorId) const;

	// 保存前の健全性チェック。問題があれば説明を詰めて false を返す。
	bool Validate(std::vector<std::string>& problems) const;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BehaviorTreeAsset, character, nodes)

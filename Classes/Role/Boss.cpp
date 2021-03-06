#include "Boss.h"

Boss::Boss() :m_nextDecisionTime(0)
{

}

Boss::~Boss()
{

}

bool Boss::init()
{
	bool ret = false;
	do{
		this->setAnchorPoint(Vec2(0.5, 0.5));
		this->initWithSpriteFrameName("23IDLE0001.png");

		Animation *idleAnim = this->createNomalAnimation("23IDLE%04d.png", 30, 25);
		this->setIdleAction(RepeatForever::create(Animate::create(idleAnim)));

		Animation *walkAnim = this->createNomalAnimation("23WALK%04d.png", 24, 25);
		this->setWalkAction(RepeatForever::create(Animate::create(walkAnim)));

		/*
		Animation *attackAnim = this->createAttackAnimation("ATTACK%05d.png", 1, 9, 25);
		this->setNomalAttackA(Sequence::create(Animate::create(attackAnim),
			CallFuncN::create(CC_CALLBACK_1(Boss::attackCallBackAction, this)),
			Role::createIdleCallbackFunc(),
			NULL));
		*/

		Animation *skillAnimA = this->createAttackAnimation("23EFFECT1%04d.png",1,31,25);
		this->setSkillAttackA(Sequence::create(
			Animate::create(skillAnimA),
			CallFuncN::create(CC_CALLBACK_1(Boss::skillCallBackAction, this)),
			Role::createIdleCallbackFunc(),
			NULL));

		Animation *skillAnimB = this->createAttackAnimation("23EFFECT2%04d.png", 1, 30, 25);
		this->setSkillAttackB(Sequence::create(
			Animate::create(skillAnimB),
			CallFuncN::create(CC_CALLBACK_1(Boss::skillCallBackAction, this)),
			Role::createIdleCallbackFunc(),
			NULL));

		Animation *hurtAnim = this->createNomalAnimation("23BEATTACK%04d.png", 13, 7);
		this->setHurtAction(Sequence::create(CallFuncN::create(
			CC_CALLBACK_1(Boss::beAttakedSoundEffectCallBackAction, this)),
			Animate::create(hurtAnim), 
			Role::createIdleCallbackFunc(), 
			NULL));

		Animation *hurtFlyAnim = this->createNomalAnimation("23BEATTACKFLY%04d.png", 6, 10);
		Animation *onGround = this->createNomalAnimation("23DEAD%04d.png", 1, 20);

		Point p1 = Point(200, 100), p2 = Point(200, 300), p3 = Point(300, 0);
		//Point p1 = Point(0, 100), p2 = Point(0, 300), p3 = Point(0, 0);
		auto bezierAnimFlyToRight = createBezierAnim(p1, p2, p3, hurtFlyAnim);
		auto bezierAnimFlyToLeft = createBezierAnim(Point(-p1.x, p1.y), Point(-p2.x, p2.y), Point(-p3.x, p3.y), hurtFlyAnim);
		this->setHurtFlyActionRight(Sequence::create(
			CallFuncN::create(CC_CALLBACK_1(Boss::beAttakedSoundEffectCallBackAction, this)),
			bezierAnimFlyToRight,
			Animate::create(onGround),
			Role::createIdleCallbackFunc(),
			NULL));
		this->setHurtFlyActionLeft(Sequence::create(
			CallFuncN::create(CC_CALLBACK_1(Boss::beAttakedSoundEffectCallBackAction, this)),
			bezierAnimFlyToLeft,
			Animate::create(onGround),
			Role::createIdleCallbackFunc(),
			NULL));

		Animation *deadAnim = this->createNomalAnimation("23DEAD%04d.png", 1, 60);
		this->setDeadAction(Sequence::create(Animate::create(deadAnim), Blink::create(3, 9), NULL));


		Size enemyShowSize = this->getSpriteFrame()->getRect().size;
		this->m_bodyBox = this->createBoundingBox(Vec2(0, 35), Size(160, 70));
		this->m_hitBox = this->createBoundingBox(Vec2(0, 35), Size(800, 320));



		//debug 参数
		//ccConfig.h
		//debug  
		/*
		auto rectBodyBox = DrawNode::create();
		Debug_DrawBoundingBox(rectBodyBox, m_bodyBox, Color4F(1.0f, 1.0f, 0.3f, 0.5f));
		//addChild(rectBodyBox);

		auto rectHitBox = DrawNode::create();
		Debug_DrawBoundingBox(rectHitBox, m_hitBox, Color4F(0.3f, 1.0f, 1.0f, 0.5f));
		//addChild(rectHitBox);
		*/
		this->setDamageStrength(10);

		ret = true;
	} while (0);

	return ret;
}

void Boss::updateSelf()
{
	this->execute(global->hero->getPosition(), global->hero->getBodyBox().actual.size.width);//对象坐标及body宽度
	if (this->getCurrActionState() == ACTION_STATE_WALK)
	{
		Vec2 location = this->getPosition();
		Vec2 direction = this->getMoveDirection();
		Vec2 expectP = location + direction;
		/*
		float maptileHeight = global->tileMap->getTileSize().height;
		if (expectP.y < 0 || expectP.y > maptileHeight * 3)
		{
		direction.y = 0;
		}
		*/

		if (false == (global->tileAllowMove(expectP)))
		{
			direction.y = 0;
			direction.x = 0;
		}

		this->setFlippedX(direction.x < 0 ? true : false);
		this->setPosition(location + direction);
		this->updateBoxes();
		this->setLocalZOrder(Director::getInstance()->getVisibleSize().height - this->getPositionY());
	}
	if (this->getCurrActionState() == ACTION_STATE_SKILL_ATTACK_A)
	{
		this->runSkillAttackA();
	}
	else if (this->getCurrActionState() == ACTION_STATE_SKILL_ATTACK_B)
	{
		this->runSkillAttackB();
	}
}

void Boss::execute(const Vec2& target, float targetBodyWidth)
{
	if (0 == m_nextDecisionTime)
	{
		this->decide(target, targetBodyWidth);
	}
	else
	{
		--m_nextDecisionTime;
	}
}

void Boss::decide(const Vec2& target, float targetBodyWidth)
{
	if (this->getCurrActionState() == ACTION_STATE_HURT || this->getCurrActionState() == ACTION_STATE_HURT_FLY)
	{
		return;
	}
	Vec2 location = this->getPosition();
	float distance = location.getDistance(target) - targetBodyWidth / 2;

	bool isFlippedX = this->isFlippedX();
	bool isOnTargetLeft = (location.x < target.x ? true : false);//方向判定
	if ((isFlippedX && isOnTargetLeft) || (!isFlippedX && !isOnTargetLeft))
		this->m_aiState = CCRANDOM_0_1() > 0.5f ? BOSS_PATROL : BOSS_IDLE;
	else
	{
		if (distance < m_eyeArea)
			this->m_aiState = (distance < m_attackArea) && ((fabsf(location.y - target.y) < 15)) ? BOSS_ATTACK : BOSS_PURSUIT;
		else
			this->m_aiState = CCRANDOM_0_1() > 0.5f ? BOSS_PATROL : BOSS_IDLE;
	}

	//如果受伤不做响应

	switch (m_aiState)
	{
	case BOSS_ATTACK:
	{
		CCRANDOM_0_1() < 0.5 ? this->runSkillAttackA(): this->runSkillAttackB();
		this->m_nextDecisionTime = 50;
		break;
	}

	case BOSS_IDLE:
	{
		this->runIdleAction();
		this->m_nextDecisionTime = CCRANDOM_0_1() * 100;
		break;
	}

	case BOSS_PATROL:
	{
		this->runWalkAction();
		this->m_moveDirection.x = CCRANDOM_MINUS1_1();
		this->m_moveDirection.y = CCRANDOM_MINUS1_1();
		m_moveDirection.x = m_moveDirection.x > 0 ? (m_moveDirection.x + velocity.x) : (m_moveDirection.x - velocity.x);
		m_moveDirection.y = m_moveDirection.y > 0 ? (m_moveDirection.y + velocity.y) : (m_moveDirection.y - velocity.y);
		this->m_nextDecisionTime = CCRANDOM_0_1() * 100;
		break;
	}

	case BOSS_PURSUIT:
	{

		this->runWalkAction();
		this->m_moveDirection = (target - location).getNormalized();
		this->setFlippedX(m_moveDirection.x < 0 ? true : false);
		m_moveDirection.x = m_moveDirection.x > 0 ? (m_moveDirection.x + velocity.x) : (m_moveDirection.x - velocity.x);
		m_moveDirection.y = m_moveDirection.y > 0 ? (m_moveDirection.y + velocity.y) : (m_moveDirection.y - velocity.y);
		this->m_nextDecisionTime = 10;
		break;
	}


	default:
		break;
	}

}


void Boss::attackCallBackAction(Node* pSender)
{
	Hero* p_hero = global->hero;
	Rect attackReck = m_hitBox.actual;//怪物攻击区域
	Rect hurtReck = p_hero->getBodyBox().actual;
	if (attackReck.intersectsRect(hurtReck))
	{
		p_hero->setAllowMove(false);
		int damage = this->getDamageStrength();
		//p_hero->runHurtAction();
		if (this->getMoveDirection().x > 0)
		{
			p_hero->setFlippedX(true);
			p_hero->runHurtFlyRightAction();
		}
		else
		{
			p_hero->setFlippedX(false);
			p_hero->runHurtFlyLeftAction();
		}

		p_hero->setCurtLifeValue(p_hero->getCurtLifeValue() - damage);
		//连击数归零
		p_hero->hitCount = 0;
	}
	if (p_hero->getCurtLifeValue() <= 0)
	{
		p_hero->runDeadAction();
		p_hero->setBodyBox(createBoundingBox(Vec2::ZERO, Size::ZERO));
	}

}

void Boss::skillCallBackAction(Node* pSender)
{
	Hero* p_hero = global->hero;
	Rect attackReck = m_hitBox.actual;//怪物攻击区域
	Rect hurtReck = p_hero->getBodyBox().actual;


	if (attackReck.intersectsRect(hurtReck))
	{
		p_hero->setAllowMove(false);
		int damage = this->getDamageStrength();

		if (getMoveDirection().x > 0)
		{
			p_hero->setFlippedX(true);
			p_hero->runHurtFlyRightAction();
		}
		else
		{
			p_hero->setFlippedX(false);
			p_hero->runHurtFlyLeftAction();
		}
		//连击数归零
		p_hero->hitCount = 0;
		p_hero->setCurtLifeValue(p_hero->getCurtLifeValue() - damage);
	}
	if (p_hero->getCurtLifeValue() <= 0)
	{
		p_hero->runDeadAction();
		p_hero->setBodyBox(createBoundingBox(Vec2::ZERO, Size::ZERO));
	}
}

//sound effect
//受伤时声音回调 
void Boss::beAttakedSoundEffectCallBackAction(Node *pSender)
{
	CocosDenshion::SimpleAudioEngine::sharedEngine()->playEffect("Sound/hit_cut01.mp3");
}



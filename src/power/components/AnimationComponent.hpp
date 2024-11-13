#pragma once

class AnimationComponent {
public:
	AnimationComponent() = default;
	virtual ~AnimationComponent() = default;
	virtual void TriggerRegistration() = 0;
	virtual void AddKeyframe() = 0;
	virtual void UpdateKeyframe() = 0;
	virtual void RemoveKeyframe() = 0;
	virtual void Freeze() = 0;
	virtual void Evaluate() = 0;
	virtual void Unfreeze() = 0;
	virtual bool IsSyncWithProvider() = 0;
	virtual void SyncWithProvider() = 0;
	virtual bool KeyframeExists() = 0;
	virtual float GetPreviousKeyframeTime() = 0;
	virtual float GetNextKeyframeTime() = 0;
};

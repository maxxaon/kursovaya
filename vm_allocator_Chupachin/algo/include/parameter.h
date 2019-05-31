#pragma once

#include <string>
#include <assert.h>

class Parameter {

// Enumerations
public:
    /*
     * Restrictions of resource. Restriction specifies the way resources are calculated
     * and checked.
     */
    enum ResourceRestriction {
        NONE = 0,
        // The sum of virtual resources to allocate should be less or equal to physical resources
        ADDITIVE = 1,
        // Virtual resource should be less then or equal to physical resource to allocate.
        SHOULD_BE_LESS_OR_EQUAL = 2,
        // Virtual resource should be greater then or equal to physical resource to allocate.
        SHOULD_BE_GREATER_OR_EQUAL = 3,
        // Virtual resource should be equal to physical resource to allocate.
        SHOULD_BE_EQUAL = 4
    };

// Constructor
public:

    Parameter(const std::string name, const ResourceRestriction resourceRestriction = NONE)
    : resourceRestriction(resourceRestriction),
      name(name) {
    }

// Useful methods
public:

    /*
     * Check whether assignment of specified resources is possible
     */
    inline bool isAssignmentPossible(unsigned virtualResource, unsigned availablePhysicalResource) {
        if ( (int)resourceRestriction < 3 )
            return virtualResource <= availablePhysicalResource;

        if ( resourceRestriction == SHOULD_BE_GREATER_OR_EQUAL )
            return virtualResource >= availablePhysicalResource;

        return virtualResource == availablePhysicalResource;
    }

    std::string getName() const {
         return name;
    }
    
    /*
     * Check whether the resource is countable, i.e. we can decrease/increase it's value
     */
    inline bool isCountable() {
        return resourceRestriction == ADDITIVE;
    }

private:

    std::string name;
    ResourceRestriction resourceRestriction;
};

class ParameterValue {
	friend class ParameterInt;
	friend class ParameterString;
	friend class ParameterReal;
public:

	virtual ~ParameterValue() {}

public:

	virtual bool compare(const ParameterValue* value) = 0;
	virtual void decrease(const ParameterValue* value) = 0;
	virtual void increase(const ParameterValue* value) = 0;
    virtual double weight() const = 0;

protected:
	union {
		int intValue;
		std::string* stringValue;
		float realValue;
	};
};

class ParameterInt: public ParameterValue {
public:
    ParameterInt(int value) {
        intValue = value;
    }

    ~ParameterInt(){}

public:

    bool compare(const ParameterValue* value) {
        return intValue >= value->intValue;
    }

    void decrease(const ParameterValue* value) {
        assert(intValue >= value->intValue);
        intValue -= value->intValue;
    }

    void increase(const ParameterValue* value) {
        intValue += value->intValue;
    }

    virtual double weight() const {
        return intValue;
    }
};

class ParameterString: public ParameterValue {
public:
    ParameterString(std::string & value) {
        stringValue = new std::string(value);
    }

    virtual ~ParameterString() {
        delete ParameterValue::stringValue;
    }

public:

    bool compare(const ParameterValue* value) {
        // String values don't have to have same values
        return true;
    }

    void decrease(const ParameterValue* value) {
    }

    void increase(const ParameterValue* value) {
    }

    virtual double weight() const {
        return 0;
    }
};

class ParameterReal: public ParameterValue {
public:
    ParameterReal(float value) {
        realValue = value;
    }

    ~ParameterReal(){}

public:

    bool compare(const ParameterValue* value) {
        return realValue >= value->realValue;
    }

    void decrease(const ParameterValue* value) {
        assert(realValue >= value->realValue);
        realValue -= value->realValue;
    }

    void increase(const ParameterValue* value) {
        realValue += value->realValue;
    }

    virtual double weight() const {
        return realValue;
    }
};

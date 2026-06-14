namespace Anjean {

  class Runtime;

  class RuntimeObject {
  public:
    explicit RuntimeObject(Runtime& runtime) : runtime_(&runtime) {}

    virtual ~RuntimeObject() = default;

    Runtime& runtime() {
      return *runtime_;
    }

    const Runtime& runtime() const {
      return *runtime_;
    }

  private:
    Runtime* runtime_;
  };
} // namespace Anjean
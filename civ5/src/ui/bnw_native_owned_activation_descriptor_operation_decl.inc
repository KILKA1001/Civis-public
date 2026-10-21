// Private to the actual configured-activation implementation in this TU.
struct detail::BnwNativeGlobalResourceWarmOwner::OwnedActivationDescriptorOperation final {
    struct State;
    std::unique_ptr<State> state;
    OwnedActivationDescriptorOperation(BnwNativeGlobalResourceWarmOwner&,
        const BnwNativeStandalonePackageLoaded&,const BnwNativeStandalonePackageLoaded::Loan&,
        const std::function<void()>&);
    ~OwnedActivationDescriptorOperation();
    OwnedActivationDescriptorOperation(const OwnedActivationDescriptorOperation&)=delete;
    OwnedActivationDescriptorOperation& operator=(const OwnedActivationDescriptorOperation&)=delete;
    const BnwNativePackageDescriptorCoreWriter::Fields& fields()const;
    std::optional<std::array<std::uint8_t,16>> guid_snapshot()const;
    const BnwNativePackageXmlParserOwner::Document& document()const;
    std::shared_ptr<OwnedActivationReceipt> new_receipt();
    void publish(const std::shared_ptr<OwnedActivationReceipt>&);
    void disable(std::uint32_t);
    void finish();
};

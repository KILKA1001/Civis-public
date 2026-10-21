// Private CPU DATA leaves for genuine591020. The sealed Paint issuer supplies
// the actual C4/record snapshots; these leaves do not issue membership or flush.
namespace detail {
class BnwLabelPaintCpuTextRecords final {
    friend class BnwLabelPaintTextLoan;
    struct Decoded final {std::uint32_t scalar,lookup,byte_count;};
    static Decoded decode_486330_known(std::string_view full_c4,
        std::uint32_t at,std::uint32_t record_end,std::uint32_t actual_flags){
        // Only canonical complete UTF8 is admitted here. Original486330 has
        // additional legacy malformed/obsolete branches; none is defaulted.
        require(!full_c4.empty()&&full_c4.back()=='\0'&&
            record_end<full_c4.size()&&at<record_end,
            "591020 actual C4 record/terminator bounds unavailable");
        const auto first=static_cast<std::uint8_t>(full_c4[at]);
        std::uint32_t scalar=0,additional=0;
        if(first<0x80u){require(first!=0,"591020 record crosses actual NUL");scalar=first;}
        else if(first>=0xc2u&&first<=0xdfu){scalar=first&0x1fu;additional=1;}
        else if(first>=0xe0u&&first<=0xefu){scalar=first&0x0fu;additional=2;}
        else if(first>=0xf0u&&first<=0xf4u){scalar=first&0x07u;additional=3;}
        else throw std::runtime_error("591020 legacy/malformed486330 lead requires separate proven branch");
        require(additional<record_end-at,"591020 incomplete486330 scalar in actual record");
        for(std::uint32_t i=1;i<=additional;++i){
            const auto next=static_cast<std::uint8_t>(full_c4[at+i]);
            require((next&0xc0u)==0x80u,"591020 malformed486330 continuation unsupported");
            scalar=(scalar<<6)|(next&0x3fu);
        }
        require((additional==0||scalar>=(additional==1?0x80u:additional==2?0x800u:0x10000u))&&
            scalar<=0x10ffffu&&(scalar<0xd800u||scalar>0xdfffu),
            "591020 noncanonical486330 scalar unsupported");
        //486330 AL is ADDITIONAL bytes, then591D... outerINC contributes1.
        //Keep original decoded scalar even when the actual mask changes lookup.
        return {scalar,(actual_flags&0x80000000u)?0x2au:scalar,additional+1};
    }
    static bool actual_style_pass_admitted(std::uint32_t selector){
        //59183C exact loopback preserves EAXstep4. No alias/default selector.
        return std::bit_cast<std::int32_t>(selector)>=0;
    }
    static std::uint32_t next_cursor(std::uint32_t cursor,std::uint32_t advance){
        return cursor+advance; //Original low32 wrap, not signed/int32 saturation.
    }
    static std::uint32_t next_peak(std::uint32_t peak,std::uint32_t cursor){
        return cursor>peak?cursor:peak; //Native unsigned JBE.
    }
    static void require_text_record_known(std::string_view full_c4,
        std::uint32_t begin,std::uint32_t byte_count,std::uint32_t discriminator,
        std::int16_t actual_truncate){
        require(discriminator==0xffffffffu,
            "591020 inline Font735AB0 needs genuine resource/payload owner");
        require(!full_c4.empty()&&full_c4.back()=='\0'&&begin<full_c4.size()&&
            byte_count<=full_c4.size()-1-begin,
            "591020 actual text record range exceeds immutable C4");
        //The oldRun has no actual16E snapshot. Positive16C requires its real
        //owner and8FC830/551CC0; it cannot be interpreted as a false byte.
        require(actual_truncate<=0,
            "591020 positive truncation requires actual16E and ellipsis producer");
    }
};
}

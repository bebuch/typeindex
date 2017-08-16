#include <string_view>
#include <type_traits>
#include <utility>


namespace typeindex::detail{


	/// \brief constexpr string length
	constexpr std::size_t strlen(char const* const str)noexcept{
		auto iter = str;
		while(*iter != '\0') ++iter;
		return static_cast< std::size_t >(iter - str);
	}

	/// \brief A simple compile time string
	template < std::size_t N >
	struct ct_string{
		/// \brief Copys c-string into array data
		template < std::size_t ... I >
		constexpr ct_string(
			char const* str, std::index_sequence< I ... >
		)noexcept: data{ *(str + I) ... } {}

		/// \brief The storage
		char const data[N];

		/// \brief Use object as string_view
		constexpr std::string_view operator()()const noexcept{
			return {data, N};
		}
	};

	/// \brief Returns the function name inclusive its template parameter T
	///        as text
	template < typename T >
	constexpr char const* r()noexcept{
		// Adjust this line for your compiler
		return __PRETTY_FUNCTION__;
	}

	/// \brief A dummy type whichs text representation is known
	struct ref;

	using namespace std::literals::string_view_literals;

	/// \brief The expected text representation of struct ref
	constexpr auto ref_text = "typeindex::detail::ref"sv;

	/// \brief Owner of the type text representation of r< ref >
	constexpr ct_string< strlen(r< ref >()) > ref_data
		{r< ref >(), std::make_index_sequence< strlen(r< ref >()) >()};

	/// \brief Length of text before the actual type begins
	constexpr std::size_t leading = ref_data().find(ref_text);

	static_assert(leading != std::string_view::npos, "Compiler not supported");

	/// \brief Length of text which is not part of the actual type
	constexpr std::size_t overhead = ref_data().size() - ref_text.size();


	/// \brief Owner of a type text representation
	template < typename T >
	struct raw_data{
		/// \brief Length of the text representation of T
		static constexpr std::size_t size = strlen(r< T >() + overhead);

		/// \brief The text representation itself
		static constexpr ct_string< size > data
			{r< T >() + leading, std::make_index_sequence< size >()};
	};

	/// \brief Text representation of T
	template < typename T >
	constexpr auto raw_name = raw_data< T >::data();

	/// \brief Cultivated version of the text representation of T
	template < typename T >
	constexpr std::string_view pretty_name()noexcept{
		// Build a new ct_string object with a cultivated version of raw_name
		// (currently raw_name is pretty enough on all tested plattforms)
		return raw_name< T >;
	}


	template <typename ...>
	struct is_valid { static constexpr bool value = true; };

    template <bool condition>
    struct when{};

	template < typename T, typename = void >
	struct tag_of: tag_of< T, when< true > >{};

	template < typename T, bool condition >
	struct tag_of< T, when< condition > >{
		using type = void;
	};

	template < typename T >
	struct tag_of< T, when< is_valid< typename T::type_tag >::value > >{
		using type = typename T::type_tag;
	};

    template < typename T > struct tag_of< T const >: tag_of< T >{};
    template < typename T > struct tag_of< T volatile >: tag_of< T >{};
    template < typename T > struct tag_of< T const volatile >: tag_of< T >{};
    template < typename T > struct tag_of< T& >: tag_of< T >{};
    template < typename T > struct tag_of< T&& >: tag_of< T >{};

	template < typename T >
	using tag_of_t = typename tag_of< T >::type;


	/// \brief Unique type to identify a typeindex::type
	struct type_tag;


	template < typename >
	constexpr bool true_c = true;


}


namespace typeindex{


	/// \brief Value representation class of a type at compile time
	template < typename T > struct type{
		/// \brief Used by is_a_type
		using type_tag = detail::type_tag;

// 		constexpr type()noexcept = default;

// 		template <
// 			typename Type,
// 			typename = std::enable_if_t< is_a_type< Type > > >
// 		constexpr type(Type)noexcept{}
	};


	/// \brief true if Type< T > is a type representation, false otherwise
	template < typename Type, typename = void >
	constexpr bool is_a_type
		= std::is_same_v< detail::type_tag, detail::tag_of_t< Type > >;


	/// \brief Type trait to get the type member as type
	template < typename Type, typename = void >
	struct unpack_type: unpack_type< Type, detail::when< true > >{
		static_assert(is_a_type< Type >);
	};

	/// \brief Type trait to get the type member as type
	template < typename T, bool condition >
	struct unpack_type< type< T >, detail::when< condition > >{
		using type = T;
	};

	/// \brief Type trait to get the type
	template < typename Type, bool condition >
	struct unpack_type< Type, detail::when< condition > >{
		using type = typename Type::type;
	};

    template < typename T > struct unpack_type< T const >: unpack_type< T >{};
    template < typename T > struct unpack_type< T volatile >: unpack_type< T >{};
    template < typename T > struct unpack_type< T const volatile >: unpack_type< T >{};
    template < typename T > struct unpack_type< T& >: unpack_type< T >{};
    template < typename T > struct unpack_type< T&& >: unpack_type< T >{};

	/// \brief Type trait to get the type from a type representator type
	template < typename T >
	using unpack_type_t = typename unpack_type< T >::type;


	/// \brief Value representation of a type at compile time
	template < typename T, typename = void >
	constexpr auto type_c = type< T >{};

	/// \brief Use encapsulated type
	template < typename Type >
	constexpr auto type_c< Type, std::enable_if_t< is_a_type< Type > > >
		= type< unpack_type_t< Type > >{};


	/// \brief Make a compile time value representation of a type with cvr
	///        stripped
	template < typename T, typename = std::enable_if_t< !is_a_type< T > > >
	constexpr type< std::remove_cv_t< std::remove_reference_t< T > > >
	typeid_(T&&)noexcept{ return{}; }

	/// \brief Overload if type is already a typeindex::type
	template < typename Type, typename = std::enable_if_t< is_a_type< Type > > >
	constexpr type< std::remove_cv_t< std::remove_reference_t<
		unpack_type_t< Type > > > >
	typeid_(Type&&)noexcept{ return{}; }


	/// \brief Make a compile time value representation of a type
	template < typename T, typename = std::enable_if_t< !is_a_type< T > > >
	constexpr type< T&& > typeid_with_cvr(T&&)noexcept{ return{}; }

	/// \brief Overload if type is already a typeindex::type
	template < typename Type, typename = std::enable_if_t< is_a_type< Type > > >
	constexpr type< unpack_type_t< Type > >
	typeid_with_cvr(Type&&)noexcept{ return{}; }


	/// \brief Callable Converter
	template < typename Converter >
	struct meta_call_with_cvr{
		/// \brief Call the Converter's data function with the type of the
		///        given agument
		template < typename T >
		constexpr typename Converter::data_type operator()(T&& v)const noexcept{
			return Converter::data(typeid_with_cvr(static_cast< T&& >(v)));
		}
	};

	/// \brief Callable Converter
	template < typename Converter >
	struct meta_call{
		/// \brief Call the Converter's data function with the type of the
		///        given agument
		template < typename T >
		constexpr typename Converter::data_type operator()(T&& v)const noexcept{
			return Converter::data(typeid_(static_cast< T&& >(v)));
		}
	};


	/// \brief The raw name of a type
	template < typename T, typename = void >
	constexpr std::string_view name_c = detail::raw_name< T >;

	/// \brief Use encapsulated type
	template < typename Type >
	constexpr std::string_view
		name_c< Type, std::enable_if_t< is_a_type< Type > > >
			= name_c< unpack_type_t< Type > >;

	/// \brief Convert class for name_c
	struct name_t{
		using data_type = std::string_view;

		template < typename T >
		static constexpr data_type data(type< T >)noexcept{
			return name_c< T >;
		}
	};

	/// \brief Callable with object, returns its types name_c
	constexpr meta_call< name_t > name{};

	/// \brief Callable with object, returns its types name_c
	constexpr meta_call_with_cvr< name_t > name_with_cvr{};


	/// \brief The pretty name of a type
	template < typename T, typename = void >
	constexpr std::string_view pretty_name_c = detail::pretty_name< T >();

	/// \brief Use encapsulated type
	template < typename Type >
	constexpr std::string_view
		pretty_name_c< Type, std::enable_if_t< is_a_type< Type > > >
			= pretty_name_c< unpack_type_t< Type > >;

	/// \brief Convert class for pretty_name_c
	struct pretty_name_t{
		using data_type = std::string_view;

		template < typename T >
		static constexpr data_type data(type< T >)noexcept{
			return pretty_name_c< T >;
		}
	};

	/// \brief Callable with object, returns its types pretty_name_c
	constexpr meta_call< pretty_name_t > pretty_name{};

	/// \brief Callable with object, returns its types pretty_name_c
	constexpr meta_call_with_cvr< pretty_name_t > pretty_name_with_cvr{};


	/// \brief Pair of name and pretty name
	struct combined{
		/// \brief Default constructor
		constexpr combined()noexcept = default;

		/// \brief Construct by a string_view
		template < typename T >
		constexpr combined(type< T >)noexcept
			: name(name_c< T >)
			, pretty_name(pretty_name_c< T >) {}

		/// \brief Runtime representation of the type
		std::string_view name;

		/// \brief Pretty runtime representation of the type
		std::string_view pretty_name;
	};

	/// \brief The pretty name of a type
	template < typename T, typename = void >
	constexpr combined combined_name_c = combined(type_c< T >);

	/// \brief Use encapsulated type
	template < typename Type >
	constexpr combined
		combined_name_c< Type, std::enable_if_t< is_a_type< Type > > >
			= combined_name_c< unpack_type_t< Type > >;

	/// \brief Convert class for combined_name_c
	struct combined_name_t{
		using data_type = combined;

		template < typename T >
		static constexpr data_type data(type< T >)noexcept{
			return combined_name_c< T >;
		}
	};

	/// \brief Callable with object, returns its types combined_name_c
	constexpr meta_call< combined_name_t > combined_name{};

	/// \brief Callable with object, returns its types combined_name_c
	constexpr meta_call_with_cvr< combined_name_t > combined_name_with_cvr{};


	constexpr bool operator==(combined const& a, combined const& b)noexcept{
		return a.name == b.name;
	}

	constexpr bool operator!=(combined const& a, combined const& b)noexcept{
		return a.name != b.name;
	}

	constexpr bool operator<(combined const& a, combined const& b)noexcept{
		return a.name < b.name;
	}

	constexpr bool operator>(combined const& a, combined const& b)noexcept{
		return a.name > b.name;
	}

	constexpr bool operator<=(combined const& a, combined const& b)noexcept{
		return a.name <= b.name;
	}

	constexpr bool operator>=(combined const& a, combined const& b)noexcept{
		return a.name >= b.name;
	}



	/// \brief A runtime type index
	template < typename Converter >
	struct basic_type_index{
	public:
		/// \brief Default constructor
		constexpr basic_type_index()noexcept = default;

		/// \brief Construct by a string_view
		template < typename T >
		constexpr basic_type_index(type< T > t)noexcept
			: data(Converter::data(t)) {}

		/// \brief Runtime representation of the type
		typename Converter::data_type data;
	};

	template < typename Converter >
	constexpr bool operator==(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data == b.data;
	}

	template < typename Converter >
	constexpr bool operator!=(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data != b.data;
	}

	template < typename Converter >
	constexpr bool operator<(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data < b.data;
	}

	template < typename Converter >
	constexpr bool operator>(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data > b.data;
	}

	template < typename Converter >
	constexpr bool operator<=(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data <= b.data;
	}

	template < typename Converter >
	constexpr bool operator>=(
		basic_type_index< Converter > const& a,
		basic_type_index< Converter > const& b
	)noexcept{
		return a.data >= b.data;
	}


	/// \brief Use the raw name of the type as runtime representation
	using type_index = basic_type_index< name_t >;

	/// \brief Use the pretty name of the type as runtime representation
	using pretty_type_index = basic_type_index< pretty_name_t >;

	/// \brief Use the name of the type as runtime representation, but also
	///        provide the pretty_name
	using combined_type_index = basic_type_index< combined_name_t >;


}

#include <functional>


namespace std{


	/// \brief Hash by name, ignore pretty name
	template <> struct hash< typeindex::combined >{
		std::size_t operator()(typeindex::combined data)const
			noexcept(noexcept(hash< std::string_view >()(data.name))){
			return hash< std::string_view >()(data.name);
		}
	};

	/// \brief Hash is the same as the hash of its name
	template < typename Converter >
	struct hash< typeindex::basic_type_index< Converter > >{
		constexpr std::size_t operator()(
			typeindex::basic_type_index< Converter > index
		)const noexcept(noexcept(
			hash< typename Converter::data_type >()(index.data)
		)){
			return hash< typename Converter::data_type >()(index.data);
		}
	};


}



#include <iostream>
#include <iomanip>
#include <boost/hana.hpp>


/// \brief Every typeindex::type is a type representator
template < typename T >
constexpr bool typeindex::is_a_type< T,
	std::enable_if_t< boost::hana::is_a< boost::hana::type_tag, T > > > = true;


template < typename > struct A{};
template < typename, typename > struct B{};
template < typename ... > struct C{};

int main(){
	using namespace typeindex;

	std::cout << std::boolalpha;
	std::cout << detail::ref_data() << '\n';

	std::cout << is_a_type< bool > << '\n';
	std::cout << is_a_type< type< bool > > << '\n';
	std::cout << is_a_type< type< type< bool > > > << '\n';
	std::cout << is_a_type< boost::hana::basic_type< bool > > << '\n';

	std::cout << name_c< type< type< bool& > > > << '\n';
	std::cout << name_c< boost::hana::basic_type< char& > > << '\n';
	std::cout << name_c< signed char& > << '\n';
	std::cout << name_c< unsigned char > << '\n';
	std::cout << name_c< int > << '\n';
	std::cout << name_c< unsigned int > << '\n';
	std::cout << name_c< float > << '\n';
	std::cout << name_c< double > << '\n';

	std::cout << name(type_c< float const >) << '\n';
	std::cout << name(boost::hana::type_c< long const& >) << '\n';
	std::cout << name(false) << '\n';
	std::cout << name('a') << '\n';

	std::cout << name_with_cvr(type_c< float const >) << '\n';
	std::cout << name_with_cvr(boost::hana::type_c< long const& >) << '\n';
	std::cout << name_with_cvr(false) << '\n';
	std::cout << name_with_cvr('a') << '\n';

// 	std::cout << name< tmpwrap< A > > << '\n';
// 	std::cout << name< tmpwrap< B > > << '\n';
// 	std::cout << name< tmpwrap< C > > << '\n';

// 	std::cout << pretty_template_name< A > << '\n';
// 	std::cout << pretty_template_name< B > << '\n';
// 	std::cout << pretty_template_name< C > << '\n';
}

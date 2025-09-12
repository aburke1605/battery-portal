import PageMeta from "../components/common/PageMeta";
import SignInForm from "../components/auth/SignInForm";

export default function SignIn() {
  return (
    <>
      <PageMeta
        title="SignIn Dashboard"
        description="React.js SignIn Dashboard Tailwind CSS Admin"
      />
      <div className="relative p-6 bg-white z-1 dark:bg-gray-900 sm:p-0">
        <div className="relative flex flex-col justify-center w-full h-screen lg:flex-row dark:bg-gray-900 sm:p-0">
        <SignInForm />
        </div>
      </div>
    </>
  );
}

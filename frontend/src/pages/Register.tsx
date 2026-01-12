import PageMeta from "../components/common/PageMeta";
import RegistrationForm from "../components/auth/RegistrationForm";

export default function Register() {
  return (
    <>
      <PageMeta
        title="Register Dashboard"
        description="React.js Register Dashboard Tailwind CSS Admin"
      />
      <div className="relative p-6 bg-white z-1 dark:bg-gray-900 sm:p-0">
        <div className="relative flex flex-col justify-center w-full h-screen lg:flex-row dark:bg-gray-900 sm:p-0">
          <RegistrationForm />
        </div>
      </div>
    </>
  );
}
